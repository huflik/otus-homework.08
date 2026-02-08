#include <gtest/gtest.h>
#include "comparator.h"
#include "block_cache.h"
#include "hasher.h"
#include <boost/filesystem.hpp>
#include <fstream>

namespace fs = boost::filesystem;

class ComparatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "comparator_test";
        fs::create_directories(temp_dir);
        
        // Создаем тестовые файлы
        CreateTestFile("file1.bin", "Hello World!");
        CreateTestFile("file2.bin", "Hello World!"); // Такой же как file1
        CreateTestFile("file3.bin", "Hello World?"); // Отличается одним символом
        CreateTestFile("file4.bin", "Hello");        // Короче
        CreateTestFile("file5.bin", "Hello World! Hello World!"); // Длиннее
        CreateTestFile("empty1.bin", ""); // Пустой
        CreateTestFile("empty2.bin", ""); // Пустой
    }
    
    void TearDown() override {
        try {
            fs::remove_all(temp_dir);
        } catch (...) {
            // Игнорируем ошибки удаления
        }
    }
    
    void CreateTestFile(const std::string& filename, const std::string& content) {
        fs::path file_path = temp_dir / filename;
        std::ofstream file(file_path.string(), std::ios::binary);
        file.write(content.data(), content.size());
    }
    
    fs::path GetTestFilePath(const std::string& filename) {
        return temp_dir / filename;
    }
    
    fs::path temp_dir;
};

TEST_F(ComparatorTest, EqualsSameFiles) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    EXPECT_TRUE(comparator.Equals(
        GetTestFilePath("file1.bin"), 
        GetTestFilePath("file2.bin")));
}

TEST_F(ComparatorTest, EqualsDifferentFiles) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    EXPECT_FALSE(comparator.Equals(
        GetTestFilePath("file1.bin"), 
        GetTestFilePath("file3.bin")));
}

TEST_F(ComparatorTest, EqualsDifferentSizes) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    // Файлы разной длины
    EXPECT_FALSE(comparator.Equals(
        GetTestFilePath("file1.bin"), 
        GetTestFilePath("file4.bin")));
}

TEST_F(ComparatorTest, EqualsEmptyFiles) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    // Два пустых файла должны быть равны
    EXPECT_TRUE(comparator.Equals(
        GetTestFilePath("empty1.bin"), 
        GetTestFilePath("empty2.bin")));
}

TEST_F(ComparatorTest, EqualsFileWithItself) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    // Файл должен быть равен сам себе
    EXPECT_TRUE(comparator.Equals(
        GetTestFilePath("file1.bin"), 
        GetTestFilePath("file1.bin")));
}

TEST_F(ComparatorTest, EqualsNonExistentFile) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    // Сравнение с несуществующим файлом должно вернуть false
    EXPECT_FALSE(comparator.Equals(
        GetTestFilePath("file1.bin"), 
        GetTestFilePath("nonexistent.bin")));
    
    EXPECT_FALSE(comparator.Equals(
        GetTestFilePath("nonexistent1.bin"), 
        GetTestFilePath("nonexistent2.bin")));
}

TEST_F(ComparatorTest, FindDuplicatesEmpty) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    // Пустой список
    std::vector<std::string> empty;
    auto result = comparator.FindDuplicates(empty);
    EXPECT_TRUE(result.empty());
    
    // Один файл
    std::vector<std::string> single = {GetTestFilePath("file1.bin").string()};
    result = comparator.FindDuplicates(single);
    EXPECT_TRUE(result.empty());
}

TEST_F(ComparatorTest, FindDuplicatesNoDuplicates) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    // Все файлы разные
    std::vector<std::string> files = {
        GetTestFilePath("file1.bin").string(),
        GetTestFilePath("file3.bin").string(),
        GetTestFilePath("file4.bin").string()
    };
    
    auto result = comparator.FindDuplicates(files);
    EXPECT_TRUE(result.empty());
}

TEST_F(ComparatorTest, FindDuplicatesWithDuplicates) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    // file1 и file2 одинаковые, file3 другой
    std::vector<std::string> files = {
        GetTestFilePath("file1.bin").string(),
        GetTestFilePath("file2.bin").string(),
        GetTestFilePath("file3.bin").string()
    };
    
    auto result = comparator.FindDuplicates(files);
    
    // Должна быть одна группа дубликатов
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].size(), 2);
    
    // Проверяем что file1 и file2 в одной группе
    EXPECT_TRUE(std::find(result[0].begin(), result[0].end(), 
                         GetTestFilePath("file1.bin").string()) != result[0].end());
    EXPECT_TRUE(std::find(result[0].begin(), result[0].end(), 
                         GetTestFilePath("file2.bin").string()) != result[0].end());
}

TEST_F(ComparatorTest, FindDuplicatesMultipleGroups) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    // Создаем больше одинаковых файлов
    CreateTestFile("copy1.bin", "Hello World!");
    CreateTestFile("copy2.bin", "Hello World!");
    CreateTestFile("copy3.bin", "Different content");
    CreateTestFile("copy4.bin", "Different content");
    
    std::vector<std::string> files = {
        GetTestFilePath("file1.bin").string(),
        GetTestFilePath("file2.bin").string(),
        GetTestFilePath("copy1.bin").string(),
        GetTestFilePath("copy2.bin").string(),
        GetTestFilePath("copy3.bin").string(),
        GetTestFilePath("copy4.bin").string()
    };
    
    auto result = comparator.FindDuplicates(files);
    
    // Должно быть 2 группы дубликатов
    ASSERT_GE(result.size(), 2);
    
    // Проверяем что каждая группа содержит дубликаты
    for (const auto& group : result) {
        EXPECT_GE(group.size(), 2);
    }
}

TEST_F(ComparatorTest, FindDuplicatesAllSame) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    // Создаем несколько одинаковых файлов
    CreateTestFile("same1.bin", "Same content");
    CreateTestFile("same2.bin", "Same content");
    CreateTestFile("same3.bin", "Same content");
    
    std::vector<std::string> files = {
        GetTestFilePath("same1.bin").string(),
        GetTestFilePath("same2.bin").string(),
        GetTestFilePath("same3.bin").string()
    };
    
    auto result = comparator.FindDuplicates(files);
    
    // Должна быть одна группа со всеми файлами
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].size(), 3);
}

TEST_F(ComparatorTest, DifferentHashAlgorithms) {
    // Тестируем с CRC32
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
        Comparator comparator(*cache);
        
        EXPECT_TRUE(comparator.Equals(
            GetTestFilePath("file1.bin"), 
            GetTestFilePath("file2.bin")));
    }
    
    // Тестируем с MD5
    {
        auto hasher = std::make_unique<Hasher>(HashType::MD5);
        auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
        Comparator comparator(*cache);
        
        EXPECT_TRUE(comparator.Equals(
            GetTestFilePath("file1.bin"), 
            GetTestFilePath("file2.bin")));
    }
}

TEST_F(ComparatorTest, DifferentBlockSizes) {
    // Маленький блок
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        auto cache = std::make_unique<BlockCache>(128, std::move(hasher)); // 128 байт блок
        Comparator comparator(*cache);
        
        // Должно работать даже с маленькими блоками
        EXPECT_TRUE(comparator.Equals(
            GetTestFilePath("file1.bin"), 
            GetTestFilePath("file2.bin")));
    }
    
    // Большой блок
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        auto cache = std::make_unique<BlockCache>(16384, std::move(hasher)); // 16KB блок
        Comparator comparator(*cache);
        
        // Должно работать с большими блоками
        EXPECT_TRUE(comparator.Equals(
            GetTestFilePath("file1.bin"), 
            GetTestFilePath("file2.bin")));
    }
}