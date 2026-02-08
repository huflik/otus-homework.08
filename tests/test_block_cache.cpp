#include <gtest/gtest.h>
#include "block_cache.h"
#include "hasher.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <memory>

namespace fs = boost::filesystem;

class BlockCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "bayan_test";
        fs::create_directories(temp_dir);
        
        CreateTestFileWithDifferentBlocks("test_diff_blocks.bin", 8192);

        CreateTestFile("same1.bin", "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
        CreateTestFile("same2.bin", "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
        CreateTestFile("diff.bin", "ZYXWVUTSRQPONMLKJIHGFEDCBA0987654321");

        CreateTestFile("empty.bin", "");

        std::string one_block_content(4096, 'X');
        CreateTestFile("one_block.bin", one_block_content);
    }
    
    void TearDown() override {
        try {
            fs::remove_all(temp_dir);
        } catch (...) {
        }
    }
    
    void CreateTestFileWithDifferentBlocks(const std::string& filename, size_t size) {
        fs::path file_path = temp_dir / filename;
        std::ofstream file(file_path.string(), std::ios::binary);

        for (size_t i = 0; i < 4096 && i < size; ++i) {
            file.put('A');
        }

        for (size_t i = 4096; i < size; ++i) {
            file.put('B');
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

TEST_F(BlockCacheTest, ConstructorValid) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    EXPECT_NO_THROW(BlockCache cache(4096, std::move(hasher)));
}

TEST_F(BlockCacheTest, ConstructorInvalidBlockSize) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    EXPECT_THROW(BlockCache cache(0, std::move(hasher)), std::invalid_argument);
}

TEST_F(BlockCacheTest, ConstructorNullHasher) {
    EXPECT_THROW(BlockCache cache(4096, nullptr), std::invalid_argument);
}

TEST_F(BlockCacheTest, GetBlockCount) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    EXPECT_EQ(cache.GetBlockCount(GetTestFilePath("test_diff_blocks.bin")), 2);
    
    EXPECT_EQ(cache.GetBlockCount(GetTestFilePath("empty.bin")), 0);
    
    EXPECT_EQ(cache.GetBlockCount(GetTestFilePath("one_block.bin")), 1);
}

TEST_F(BlockCacheTest, GetBlockCountCaching) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("test_diff_blocks.bin");
    
    size_t count1 = cache.GetBlockCount(file);
    
    size_t count2 = cache.GetBlockCount(file);
    
    EXPECT_EQ(count1, count2);
    EXPECT_EQ(count1, 2);
}

TEST_F(BlockCacheTest, GetBlockCountInvalidFile) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    EXPECT_THROW(cache.GetBlockCount("non_existent_file.bin"), std::runtime_error);
}

TEST_F(BlockCacheTest, GetBlockHashCaching) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("test_diff_blocks.bin");
    
    std::string hash_block0_first = cache.GetBlockHash(file, 0);
    std::string hash_block1_first = cache.GetBlockHash(file, 1);
    
    EXPECT_NE(hash_block0_first, hash_block1_first);
    
    std::string hash_block0_second = cache.GetBlockHash(file, 0);
    std::string hash_block1_second = cache.GetBlockHash(file, 1);
    
    EXPECT_EQ(hash_block0_first, hash_block0_second);
    EXPECT_EQ(hash_block1_first, hash_block1_second);
}

TEST_F(BlockCacheTest, GetBlockHashSameContent) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file1 = GetTestFilePath("same1.bin");
    auto file2 = GetTestFilePath("same2.bin");
    
    std::string hash1 = cache.GetBlockHash(file1, 0);
    std::string hash2 = cache.GetBlockHash(file2, 0);
    
    EXPECT_EQ(hash1, hash2);
}

TEST_F(BlockCacheTest, GetBlockHashDifferentContent) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file1 = GetTestFilePath("same1.bin");
    auto file2 = GetTestFilePath("diff.bin");
    
    std::string hash1 = cache.GetBlockHash(file1, 0);
    std::string hash2 = cache.GetBlockHash(file2, 0);
    
    EXPECT_NE(hash1, hash2);
}

TEST_F(BlockCacheTest, GetBlockHashInvalidBlock) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("one_block.bin"); 
    
    EXPECT_NO_THROW(cache.GetBlockHash(file, 0));
    
    EXPECT_NO_THROW({
        std::string hash = cache.GetBlockHash(file, 1);
        EXPECT_FALSE(hash.empty());
    });
}

TEST_F(BlockCacheTest, FileHandleCaching) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("test_diff_blocks.bin");

    std::string hash1 = cache.GetBlockHash(file, 0);

    std::string hash2 = cache.GetBlockHash(file, 0);
    
    EXPECT_EQ(hash1, hash2);
}

TEST_F(BlockCacheTest, MultipleFiles) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file1 = GetTestFilePath("same1.bin");
    auto file2 = GetTestFilePath("diff.bin");

    EXPECT_NO_THROW(cache.GetBlockHash(file1, 0));
    EXPECT_NO_THROW(cache.GetBlockHash(file2, 0));
}

TEST_F(BlockCacheTest, DifferentHashTypes) {
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        BlockCache cache(4096, std::move(hasher));
        
        auto file = GetTestFilePath("same1.bin");
        std::string hash = cache.GetBlockHash(file, 0);

        EXPECT_EQ(hash.size(), 8);
    }
    
    {
        auto hasher = std::make_unique<Hasher>(HashType::MD5);
        BlockCache cache(4096, std::move(hasher));
        
        auto file = GetTestFilePath("same1.bin");
        std::string hash = cache.GetBlockHash(file, 0);
        
        EXPECT_EQ(hash.size(), 32);
    }
}

TEST_F(BlockCacheTest, BlockKeyHash) {
    BlockKey key1{"file1", 0};
    BlockKey key2{"file1", 0};
    BlockKey key3{"file1", 1};
    BlockKey key4{"file2", 0};
    
    BlockKeyHash hasher;

    EXPECT_EQ(hasher(key1), hasher(key2));
}

TEST_F(BlockCacheTest, BlockKeyEquality) {
    BlockKey key1{"file1", 0};
    BlockKey key2{"file1", 0};
    BlockKey key3{"file1", 1};
    BlockKey key4{"file2", 0};
    
    EXPECT_TRUE(key1 == key2);
    EXPECT_FALSE(key1 == key3);
    EXPECT_FALSE(key1 == key4);
    EXPECT_FALSE(key3 == key4);
}

TEST_F(BlockCacheTest, DifferentBlockSizes) {
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        BlockCache cache(1024, std::move(hasher));
        
        auto file = GetTestFilePath("test_diff_blocks.bin");
        size_t blocks = cache.GetBlockCount(file);
        EXPECT_EQ(blocks, 8);
    }
}

TEST_F(BlockCacheTest, ReadEmptyFile) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("empty.bin");

    EXPECT_EQ(cache.GetBlockCount(file), 0);

    EXPECT_NO_THROW({
        std::string hash = cache.GetBlockHash(file, 0);
        EXPECT_FALSE(hash.empty());
    });
}