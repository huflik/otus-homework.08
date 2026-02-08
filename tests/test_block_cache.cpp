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
        // Создаем временную директорию для тестовых файлов
        temp_dir = fs::temp_directory_path() / "bayan_test";
        fs::create_directories(temp_dir);
        
        // Создаем тестовые файлы с РАЗНЫМ содержимым в разных блоках
        // Файл с явно разным содержимым в первых двух блоках
        CreateTestFileWithDifferentBlocks("test_diff_blocks.bin", 8192);
        
        // Файлы для проверки одинакового содержимого
        CreateTestFile("same1.bin", "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
        CreateTestFile("same2.bin", "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
        CreateTestFile("diff.bin", "ZYXWVUTSRQPONMLKJIHGFEDCBA0987654321");
        
        // Пустой файл
        CreateTestFile("empty.bin", "");
        
        // Файл размером ровно в один блок
        std::string one_block_content(4096, 'X');
        CreateTestFile("one_block.bin", one_block_content);
    }
    
    void TearDown() override {
        // Удаляем временную директорию
        try {
            fs::remove_all(temp_dir);
        } catch (...) {
            // Игнорируем ошибки удаления
        }
    }
    
    void CreateTestFileWithDifferentBlocks(const std::string& filename, size_t size) {
        fs::path file_path = temp_dir / filename;
        std::ofstream file(file_path.string(), std::ios::binary);
        
        // Первый блок: буквы 'A'
        for (size_t i = 0; i < 4096 && i < size; ++i) {
            file.put('A');
        }
        
        // Второй блок: буквы 'B'
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
    
    // Файл 8192 байт = 2 блока
    EXPECT_EQ(cache.GetBlockCount(GetTestFilePath("test_diff_blocks.bin")), 2);
    
    // Файл 0 байт = 0 блоков
    EXPECT_EQ(cache.GetBlockCount(GetTestFilePath("empty.bin")), 0);
    
    // Файл 4096 байт = 1 блок
    EXPECT_EQ(cache.GetBlockCount(GetTestFilePath("one_block.bin")), 1);
}

TEST_F(BlockCacheTest, GetBlockCountCaching) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("test_diff_blocks.bin");
    
    // Первый вызов должен вычислить значение
    size_t count1 = cache.GetBlockCount(file);
    
    // Второй вызов должен взять из кэша
    size_t count2 = cache.GetBlockCount(file);
    
    EXPECT_EQ(count1, count2);
    EXPECT_EQ(count1, 2); // 8192 / 4096 = 2 блока
}

TEST_F(BlockCacheTest, GetBlockCountInvalidFile) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    EXPECT_THROW(cache.GetBlockCount("non_existent_file.bin"), std::runtime_error);
}

TEST_F(BlockCacheTest, GetBlockHashCaching) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("test_diff_blocks.bin"); // 2 блока с разным содержимым
    
    // Первый вызов - хэш вычисляется и кэшируется
    std::string hash_block0_first = cache.GetBlockHash(file, 0);
    std::string hash_block1_first = cache.GetBlockHash(file, 1);
    
    // Разные блоки с разным содержимым должны давать разные хэши
    // (первый блок 'A', второй блок 'B')
    EXPECT_NE(hash_block0_first, hash_block1_first);
    
    // Второй вызов - должен взять из кэша
    std::string hash_block0_second = cache.GetBlockHash(file, 0);
    std::string hash_block1_second = cache.GetBlockHash(file, 1);
    
    // Кэшированные значения должны совпадать с первоначальными
    EXPECT_EQ(hash_block0_first, hash_block0_second);
    EXPECT_EQ(hash_block1_first, hash_block1_second);
}

TEST_F(BlockCacheTest, GetBlockHashSameContent) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file1 = GetTestFilePath("same1.bin");
    auto file2 = GetTestFilePath("same2.bin");
    
    // Файлы с одинаковым содержимым должны давать одинаковый хэш
    std::string hash1 = cache.GetBlockHash(file1, 0);
    std::string hash2 = cache.GetBlockHash(file2, 0);
    
    EXPECT_EQ(hash1, hash2);
}

TEST_F(BlockCacheTest, GetBlockHashDifferentContent) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file1 = GetTestFilePath("same1.bin");
    auto file2 = GetTestFilePath("diff.bin");
    
    // Файлы с разным содержимым должны давать разные хэши
    std::string hash1 = cache.GetBlockHash(file1, 0);
    std::string hash2 = cache.GetBlockHash(file2, 0);
    
    EXPECT_NE(hash1, hash2);
}

TEST_F(BlockCacheTest, GetBlockHashInvalidBlock) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("one_block.bin"); // 1 блок
    
    // Запрос блока 0 - OK (единственный блок)
    EXPECT_NO_THROW(cache.GetBlockHash(file, 0));
    
    // Запрос блока 1 - выходит за границы
    // Реализация может либо бросить исключение, либо вернуть хэш для нулевых данных
    // Мы просто проверяем что не падает с критической ошибкой
    EXPECT_NO_THROW({
        std::string hash = cache.GetBlockHash(file, 1);
        // Хэш должен быть валидным (возможно хэш нулевых данных)
        EXPECT_FALSE(hash.empty());
    });
}

TEST_F(BlockCacheTest, FileHandleCaching) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("test_diff_blocks.bin");
    
    // Первый вызов должен открыть файл
    std::string hash1 = cache.GetBlockHash(file, 0);
    
    // Второй вызов должен использовать уже открытый файл
    std::string hash2 = cache.GetBlockHash(file, 0);
    
    EXPECT_EQ(hash1, hash2);
}

TEST_F(BlockCacheTest, MultipleFiles) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file1 = GetTestFilePath("same1.bin");
    auto file2 = GetTestFilePath("diff.bin");
    
    // Работаем с двумя файлами
    EXPECT_NO_THROW(cache.GetBlockHash(file1, 0));
    EXPECT_NO_THROW(cache.GetBlockHash(file2, 0));
}

TEST_F(BlockCacheTest, DifferentHashTypes) {
    // Тестируем с CRC32
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        BlockCache cache(4096, std::move(hasher));
        
        auto file = GetTestFilePath("same1.bin");
        std::string hash = cache.GetBlockHash(file, 0);
        
        // CRC32 должен давать 8-символьный hex хэш
        EXPECT_EQ(hash.size(), 8);
    }
    
    // Тестируем с MD5
    {
        auto hasher = std::make_unique<Hasher>(HashType::MD5);
        BlockCache cache(4096, std::move(hasher));
        
        auto file = GetTestFilePath("same1.bin");
        std::string hash = cache.GetBlockHash(file, 0);
        
        // MD5 должен давать 32-символьный hex хэш
        EXPECT_EQ(hash.size(), 32);
    }
}

TEST_F(BlockCacheTest, BlockKeyHash) {
    BlockKey key1{"file1", 0};
    BlockKey key2{"file1", 0};
    BlockKey key3{"file1", 1};
    BlockKey key4{"file2", 0};
    
    BlockKeyHash hasher;
    
    // Одинаковые ключи - одинаковый хэш
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
    // Маленький блок
    {
        auto hasher = std::make_unique<Hasher>(HashType::CRC32);
        BlockCache cache(1024, std::move(hasher)); // 1KB блок
        
        auto file = GetTestFilePath("test_diff_blocks.bin"); // 8192 байт
        size_t blocks = cache.GetBlockCount(file);
        EXPECT_EQ(blocks, 8); // 8192 / 1024 = 8
    }
}

TEST_F(BlockCacheTest, ReadEmptyFile) {
    auto hasher = std::make_unique<Hasher>(HashType::CRC32);
    BlockCache cache(4096, std::move(hasher));
    
    auto file = GetTestFilePath("empty.bin");
    
    // У пустого файла 0 блоков
    EXPECT_EQ(cache.GetBlockCount(file), 0);
    
    // Запрос любого блока
    EXPECT_NO_THROW({
        std::string hash = cache.GetBlockHash(file, 0);
        // Хэш должен быть валидным (хэш нулевых данных)
        EXPECT_FALSE(hash.empty());
    });
}