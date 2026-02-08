#include <gtest/gtest.h>
#include "hasher.h"
#include <string>
#include <cstring>

TEST(HasherTest, Constructor) {
    EXPECT_NO_THROW(Hasher(HashType::CRC32));
    EXPECT_NO_THROW(Hasher(HashType::MD5));
}

TEST(HasherTest, HashBlockCrc32) {
    Hasher hasher(HashType::CRC32);
    
    std::string test_data = "Hello, World!";
    auto hash = hasher.HashBlock(test_data.c_str(), test_data.size());
    EXPECT_EQ(hash.size(), 8);
    
    auto hash2 = hasher.HashBlock(test_data.c_str(), test_data.size());
    EXPECT_EQ(hash, hash2);
}

TEST(HasherTest, HashBlockMd5) {
    Hasher hasher(HashType::MD5);
    
    std::string test_data = "Hello, World!";
    auto hash = hasher.HashBlock(test_data.c_str(), test_data.size());
    EXPECT_EQ(hash.size(), 32);
    
    auto hash2 = hasher.HashBlock(test_data.c_str(), test_data.size());
    EXPECT_EQ(hash, hash2);
}