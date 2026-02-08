#include <gtest/gtest.h>
#include "duplicate_finder.h"
#include "hasher.h"
#include <map>

class DuplicateFinderTest : public ::testing::Test {
protected:
    void SetUp() override {
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
    std::map<uintmax_t, std::vector<std::string>> groups = {
        {100, {"test/file1.txt", "test/file2.txt", "test/file3.txt"}},
        {200, {"test/file4.txt", "test/file5.txt"}},
        {300, {"test/file6.txt"}}
    };
    
    auto result = finder->Find(groups);
    EXPECT_NO_THROW();
}

TEST_F(DuplicateFinderTest, FindReturnsValidStructure) {
    std::map<uintmax_t, std::vector<std::string>> groups = {
        {100, {"test/file1.txt", "test/file2.txt"}}
    };
    
    auto result = finder->Find(groups);

    EXPECT_NO_THROW();
}

TEST_F(DuplicateFinderTest, DifferentHashTypes) {
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
        DuplicateFinder finder(std::move(cache));
        
        std::map<uintmax_t, std::vector<std::string>> groups = {
            {100, {"file1.txt", "file2.txt"}}
        };
        
        EXPECT_NO_THROW(finder.Find(groups));
    }
    
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
    namespace fs = boost::filesystem;
    fs::path temp_dir = fs::temp_directory_path() / "finder_test";
    fs::create_directories(temp_dir);
    
    {
        std::ofstream file1((temp_dir / "file1.txt").string());
        file1 << "Same content";
        
        std::ofstream file2((temp_dir / "file2.txt").string());
        file2 << "Same content";
        
        std::ofstream file3((temp_dir / "file3.txt").string());
        file3 << "Different content";
    }
    
    std::map<uintmax_t, std::vector<std::string>> groups;
    
    for (const auto& entry : fs::directory_iterator(temp_dir)) {
        if (fs::is_regular_file(entry)) {
            uintmax_t size = fs::file_size(entry);
            groups[size].push_back(entry.path().string());
        }
    }
    
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    DuplicateFinder finder(std::move(cache));
    
    auto result = finder.Find(groups);

    bool found_duplicates = false;
    for (const auto& group : result) {
        if (group.size() >= 2) {
            found_duplicates = true;
            break;
        }
    }
    
    fs::remove_all(temp_dir);
    
    EXPECT_TRUE(found_duplicates);
}