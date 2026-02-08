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

        CreateTestFile("file1.bin", "Hello World!");
        CreateTestFile("file2.bin", "Hello World!"); 
        CreateTestFile("file3.bin", "Hello World?"); 
        CreateTestFile("file4.bin", "Hello");
        CreateTestFile("file5.bin", "Hello World! Hello World!");
        CreateTestFile("empty1.bin", "");
        CreateTestFile("empty2.bin", "");
    }
    
    void TearDown() override {
        try {
            fs::remove_all(temp_dir);
        } catch (...) {
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
    
    EXPECT_FALSE(comparator.Equals(
        GetTestFilePath("file1.bin"), 
        GetTestFilePath("file4.bin")));
}

TEST_F(ComparatorTest, EqualsEmptyFiles) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    EXPECT_TRUE(comparator.Equals(
        GetTestFilePath("empty1.bin"), 
        GetTestFilePath("empty2.bin")));
}

TEST_F(ComparatorTest, EqualsFileWithItself) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    EXPECT_TRUE(comparator.Equals(
        GetTestFilePath("file1.bin"), 
        GetTestFilePath("file1.bin")));
}

TEST_F(ComparatorTest, EqualsNonExistentFile) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
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

    std::vector<std::string> empty;
    auto result = comparator.FindDuplicates(empty);
    EXPECT_TRUE(result.empty());

    std::vector<std::string> single = {GetTestFilePath("file1.bin").string()};
    result = comparator.FindDuplicates(single);
    EXPECT_TRUE(result.empty());
}

TEST_F(ComparatorTest, FindDuplicatesNoDuplicates) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);

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

    std::vector<std::string> files = {
        GetTestFilePath("file1.bin").string(),
        GetTestFilePath("file2.bin").string(),
        GetTestFilePath("file3.bin").string()
    };
    
    auto result = comparator.FindDuplicates(files);
    
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].size(), 2);
    
    EXPECT_TRUE(std::find(result[0].begin(), result[0].end(), 
                         GetTestFilePath("file1.bin").string()) != result[0].end());
    EXPECT_TRUE(std::find(result[0].begin(), result[0].end(), 
                         GetTestFilePath("file2.bin").string()) != result[0].end());
}

TEST_F(ComparatorTest, FindDuplicatesMultipleGroups) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
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
    
    ASSERT_GE(result.size(), 2);
    
    for (const auto& group : result) {
        EXPECT_GE(group.size(), 2);
    }
}

TEST_F(ComparatorTest, FindDuplicatesAllSame) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
    Comparator comparator(*cache);
    
    CreateTestFile("same1.bin", "Same content");
    CreateTestFile("same2.bin", "Same content");
    CreateTestFile("same3.bin", "Same content");
    
    std::vector<std::string> files = {
        GetTestFilePath("same1.bin").string(),
        GetTestFilePath("same2.bin").string(),
        GetTestFilePath("same3.bin").string()
    };
    
    auto result = comparator.FindDuplicates(files);
    
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].size(), 3);
}

TEST_F(ComparatorTest, DifferentHashAlgorithms) {
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        auto cache = std::make_unique<BlockCache>(4096, std::move(hasher));
        Comparator comparator(*cache);
        
        EXPECT_TRUE(comparator.Equals(
            GetTestFilePath("file1.bin"), 
            GetTestFilePath("file2.bin")));
    }
    
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
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        auto cache = std::make_unique<BlockCache>(128, std::move(hasher));
        Comparator comparator(*cache);
        
        EXPECT_TRUE(comparator.Equals(
            GetTestFilePath("file1.bin"), 
            GetTestFilePath("file2.bin")));
    }
    
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        auto cache = std::make_unique<BlockCache>(16384, std::move(hasher));
        Comparator comparator(*cache);
        
        EXPECT_TRUE(comparator.Equals(
            GetTestFilePath("file1.bin"), 
            GetTestFilePath("file2.bin")));
    }
}