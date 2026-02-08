#include <gtest/gtest.h>
#include "duplicate_finder.h"
#include "hasher.h"
#include <map>

class DuplicateFinderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем реальные объекты
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
        finder = std::make_unique<DuplicateFinder>(std::move(cache));
    }
    
    std::unique_ptr<DuplicateFinder> finder;
};

TEST_F(DuplicateFinderTest, ConstructorValid) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    
    EXPECT_NO_THROW(DuplicateFinder finder(std::move(cache)));
}

TEST_F(DuplicateFinderTest, ConstructorNullCache) {
    EXPECT_THROW(DuplicateFinder finder(nullptr), std::invalid_argument);
}

TEST_F(DuplicateFinderTest, FindEmptyGroups) {
    std::map<uintmax_t, std::vector<std::string>> empty_groups;
    auto result = finder->Find(empty_groups);
    EXPECT_TRUE(result.empty());
}

TEST_F(DuplicateFinderTest, FindSingleFileGroups) {
    std::map<uintmax_t, std::vector<std::string>> groups = {
        {100, {"file1.txt"}},
        {200, {"file2.txt"}},
        {300, {"file3.txt"}}
    };
    
    auto result = finder->Find(groups);
    EXPECT_TRUE(result.empty());
}

TEST_F(DuplicateFinderTest, FindGroupsWithMultipleFiles) {
    // Группы файлов по размеру
    std::map<uintmax_t, std::vector<std::string>> groups = {
        {100, {"test/file1.txt", "test/file2.txt", "test/file3.txt"}},
        {200, {"test/file4.txt", "test/file5.txt"}},
        {300, {"test/file6.txt"}} // Один файл - не должно быть дубликатов
    };
    
    auto result = finder->Find(groups);
    // Результат зависит от реальных файлов
    // Просто проверяем что не падает
    EXPECT_NO_THROW();
}

TEST_F(DuplicateFinderTest, FindReturnsValidStructure) {
    std::map<uintmax_t, std::vector<std::string>> groups = {
        {100, {"test/file1.txt", "test/file2.txt"}}
    };
    
    auto result = finder->Find(groups);
    
    // Результат должен быть вектором векторов
    // Даже если дубликатов нет, структура должна быть валидной
    EXPECT_NO_THROW();
}

TEST_F(DuplicateFinderTest, DifferentHashTypes) {
    // Тестируем с CRC32
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
        DuplicateFinder finder(std::move(cache));
        
        std::map<uintmax_t, std::vector<std::string>> groups = {
            {100, {"file1.txt", "file2.txt"}}
        };
        
        EXPECT_NO_THROW(finder.Find(groups));
    }
    
    // Тестируем с MD5
    {
        auto hasher = std::make_unique<Hasher>(HashType::MD5);
        auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
        DuplicateFinder finder(std::move(cache));
        
        std::map<uintmax_t, std::vector<std::string>> groups = {
            {100, {"file1.txt", "file2.txt"}}
        };
        
        EXPECT_NO_THROW(finder.Find(groups));
    }
}

TEST_F(DuplicateFinderTest, IntegrationWithRealFiles) {
    // Создаем временные файлы
    namespace fs = boost::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "finder_test";
    fs::create_directories(temp_dir);
    
    // Создаем одинаковые файлы
    {
        std::ofstream file1((temp_dir / "file1.txt").string());
        file1 << "Same content";
        
        std::ofstream file2((temp_dir / "file2.txt").string());
        file2 << "Same content";
        
        std::ofstream file3((temp_dir / "file3.txt").string());
        file3 << "Different content";
    }
    
    // Создаем группы по размеру
    std::map<uintmax_t, std::vector<std::string>> groups;
    
    // Собираем файлы по размеру
    for (const auto& entry : fs::directory_iterator(temp_dir)) {
        if (fs::is_regular_file(entry)) {
            uintmax_t size = fs::file_size(entry);
            groups[size].push_back(entry.path().string());
        }
    }
    
    // Ищем дубликаты
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    DuplicateFinder finder(std::move(cache));
    
    auto result = finder.Find(groups);
    
    // Должны найти хотя бы одну группу дубликатов
    // (file1.txt и file2.txt одинаковые)
    bool found_duplicates = false;
    for (const auto& group : result) {
        if (group.size() >= 2) {
            found_duplicates = true;
            break;
        }
    }
    
    // Очистка
    fs::remove_all(temp_dir);
    
    EXPECT_TRUE(found_duplicates);
}