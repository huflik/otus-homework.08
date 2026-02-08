#include <gtest/gtest.h>
#include "scanner.h"
#include "config.h"
#include <boost/filesystem.hpp>
#include <fstream>

namespace fs = boost::filesystem;

class ScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем временную директорию для тестов
        test_root = fs::temp_directory_path() / "scanner_test";
        fs::create_directories(test_root);
        
        // Создаем тестовую структуру
        CreateTestFiles();
    }
    
    void TearDown() override {
        try {
            fs::remove_all(test_root);
        } catch (...) {
            // Игнорируем ошибки удаления
        }
    }
    
    void CreateTestFiles() {
        // Файлы разного размера
        CreateFile("small.txt", 50);      // Меньше min_size
        CreateFile("medium.txt", 2000);   // Больше min_size
        CreateFile("large.txt", 5000);    // Больше min_size
        
        // Файлы с разными расширениями
        CreateFile("document.pdf", 3000);
        CreateFile("image.jpg", 4000);
        CreateFile("text.txt", 3000);
        
        // Вложенные директории
        fs::create_directories(test_root / "subdir1");
        fs::create_directories(test_root / "subdir2");
        
        CreateFile("subdir1/file1.txt", 3000);
        CreateFile("subdir1/file2.txt", 3000);
        CreateFile("subdir2/file3.txt", 3000);
    }
    
    void CreateFile(const std::string& relative_path, size_t size) {
        fs::path file_path = test_root / relative_path;
        std::ofstream file(file_path.string(), std::ios::binary);
        
        // Заполняем файл данными
        for (size_t i = 0; i < size; ++i) {
            file.put(static_cast<char>(i % 256));
        }
    }
    
    fs::path test_root;
};

TEST_F(ScannerTest, ScanBasic) {
    Config config;
    config.include_dirs.push_back(test_root);
    config.min_file_size = 100; // 100 байт
    
    Scanner scanner(config);
    auto result = scanner.Scan();
    
    // Должны найти файлы больше 100 байт
    EXPECT_FALSE(result.empty());
    
    // Проверяем что нашли файлы нужного размера
    bool found_medium = false;
    bool found_large = false;
    
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            if (file.find("medium.txt") != std::string::npos) {
                found_medium = true;
            }
            if (file.find("large.txt") != std::string::npos) {
                found_large = true;
            }
        }
    }
    
    EXPECT_TRUE(found_medium);
    EXPECT_TRUE(found_large);
}

TEST_F(ScannerTest, ScanWithMinSize) {
    Config config;
    config.include_dirs.push_back(test_root);
    config.min_file_size = 3000; // 3000 байт
    
    Scanner scanner(config);
    auto result = scanner.Scan();
    
    // Должны найти только файлы >= 3000 байт
    for (const auto& [size, files] : result) {
        EXPECT_GE(size, config.min_file_size);
        
        // Не должно быть small.txt (50 байт)
        for (const auto& file : files) {
            EXPECT_TRUE(file.find("small.txt") == std::string::npos);
        }
    }
}

TEST_F(ScannerTest, ScanWithMasks) {
    Config config;
    config.include_dirs.push_back(test_root);
    config.min_file_size = 1;
    config.masks = {"*.txt", "*.jpg"};
    
    Scanner scanner(config);
    auto result = scanner.Scan();
    
    // Должны найти только .txt и .jpg файлы
    bool found_txt = false;
    bool found_jpg = false;
    bool found_pdf = false;
    
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            if (file.find(".txt") != std::string::npos) {
                found_txt = true;
            } else if (file.find(".jpg") != std::string::npos) {
                found_jpg = true;
            } else if (file.find(".pdf") != std::string::npos) {
                found_pdf = true;
            }
        }
    }
    
    EXPECT_TRUE(found_txt);
    EXPECT_TRUE(found_jpg);
    EXPECT_FALSE(found_pdf); // .pdf не должен быть найден
}

TEST_F(ScannerTest, ScanMultipleDirectories) {
    Config config;
    config.include_dirs.push_back(test_root / "subdir1");
    config.include_dirs.push_back(test_root / "subdir2");
    config.min_file_size = 1;
    
    Scanner scanner(config);
    auto result = scanner.Scan();
    
    // Должны найти файлы из обеих директорий
    bool found_subdir1 = false;
    bool found_subdir2 = false;
    
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            if (file.find("subdir1") != std::string::npos) {
                found_subdir1 = true;
            }
            if (file.find("subdir2") != std::string::npos) {
                found_subdir2 = true;
            }
        }
    }
    
    EXPECT_TRUE(found_subdir1);
    EXPECT_TRUE(found_subdir2);
}

TEST_F(ScannerTest, ScanWithDepth) {
    // Создаем вложенную структуру
    fs::create_directories(test_root / "level1" / "level2" / "level3");
    CreateFile("level1/level2/level3/deep.txt", 1000);
    
    // depth = 1 (только level1)
    {
        Config config;
        config.include_dirs.push_back(test_root);
        config.min_file_size = 1;
        config.depth = 1;
        
        Scanner scanner(config);
        auto result = scanner.Scan();
        
        // deep.txt не должен быть найден (на глубине 3)
        bool found_deep = false;
        for (const auto& [size, files] : result) {
            for (const auto& file : files) {
                if (file.find("deep.txt") != std::string::npos) {
                    found_deep = true;
                    break;
                }
            }
        }
        
        EXPECT_FALSE(found_deep);
    }
    
    // depth = 3 (должен найти deep.txt)
    {
        Config config;
        config.include_dirs.push_back(test_root);
        config.min_file_size = 1;
        config.depth = 3;
        
        Scanner scanner(config);
        auto result = scanner.Scan();
        
        bool found_deep = false;
        for (const auto& [size, files] : result) {
            for (const auto& file : files) {
                if (file.find("deep.txt") != std::string::npos) {
                    found_deep = true;
                    break;
                }
            }
        }
        
        EXPECT_TRUE(found_deep);
    }
}

TEST_F(ScannerTest, ScanExcludeDirectories) {
    Config config;
    config.include_dirs.push_back(test_root);
    config.min_file_size = 1;
    config.exclude_dirs.push_back(test_root / "subdir1");
    
    Scanner scanner(config);
    auto result = scanner.Scan();
    
    // Не должно быть файлов из subdir1
    bool found_excluded = false;
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            if (file.find("subdir1") != std::string::npos) {
                found_excluded = true;
                break;
            }
        }
    }
    
    EXPECT_FALSE(found_excluded);
}

TEST_F(ScannerTest, ScanEmptyDirectory) {
    // Создаем пустую директорию
    fs::path empty_dir = fs::temp_directory_path() / "empty_test";
    fs::create_directories(empty_dir);
    
    Config config;
    config.include_dirs.push_back(empty_dir);
    config.min_file_size = 1;
    
    Scanner scanner(config);
    auto result = scanner.Scan();
    
    // Результат должен быть пустым
    EXPECT_TRUE(result.empty());
    
    // Очистка
    fs::remove_all(empty_dir);
}

TEST_F(ScannerTest, ScanNonExistentDirectory) {
    Config config;
    config.include_dirs.push_back("/non/existent/directory");
    config.min_file_size = 1;
    
    Scanner scanner(config);
    
    // Не должно падать, должно просто пропустить директорию
    EXPECT_NO_THROW(scanner.Scan());
}

TEST_F(ScannerTest, ScanGroupsBySize) {
    Config config;
    config.include_dirs.push_back(test_root);
    config.min_file_size = 1;
    
    Scanner scanner(config);
    auto result = scanner.Scan();
    
    // Результат должен быть сгруппирован по размеру
    for (const auto& [size, files] : result) {
        EXPECT_FALSE(files.empty());
        
        // Все файлы в группе должны иметь одинаковый размер
        for (const auto& file : files) {
            try {
                uintmax_t file_size = fs::file_size(file);
                EXPECT_EQ(file_size, size);
            } catch (...) {
                // Игнорируем ошибки доступа к файлам
            }
        }
    }
}

TEST_F(ScannerTest, ScanDuplicatePaths) {
    // Добавляем одну и ту же директорию дважды
    Config config;
    config.include_dirs.push_back(test_root);
    config.include_dirs.push_back(test_root); // Дубликат
    config.min_file_size = 1;
    
    Scanner scanner(config);
    auto result = scanner.Scan();
    
    // Не должно быть дубликатов файлов
    std::set<std::string> unique_files;
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            unique_files.insert(file);
        }
    }
    
    // Количество уникальных файлов должно быть равно общему количеству
    size_t total_files = 0;
    for (const auto& [size, files] : result) {
        total_files += files.size();
    }
    
    EXPECT_EQ(unique_files.size(), total_files);
}