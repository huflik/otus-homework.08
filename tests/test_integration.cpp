#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include "parser.h"
#include "scanner.h"
#include "duplicate_finder.h"
#include "hasher.h"
#include "block_cache.h"

namespace fs = boost::filesystem;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем временную директорию для тестов
        test_root = fs::temp_directory_path() / "bayan_integration_test";
        fs::create_directories(test_root);
        
        // Создаем структуру тестовых файлов
        CreateTestStructure();
    }
    
    void TearDown() override {
        // Удаляем временную директорию
        try {
            fs::remove_all(test_root);
        } catch (...) {
            // Игнорируем ошибки удаления
        }
    }
    
    void CreateTestStructure() {
        // Директория 1: несколько файлов
        fs::path dir1 = test_root / "dir1";
        fs::create_directories(dir1);
        
        // Файлы с одинаковым содержимым (дубликаты)
        CreateFile(dir1 / "file1.txt", "This is duplicate file content. Lorem ipsum dolor sit amet.");
        CreateFile(dir1 / "file2.txt", "This is duplicate file content. Lorem ipsum dolor sit amet."); // Дубликат file1
        CreateFile(dir1 / "file3.txt", "This is unique file content. Consectetur adipiscing elit."); // Уникальный
        
        // Директория 2: больше дубликатов
        fs::path dir2 = test_root / "dir2";
        fs::create_directories(dir2);
        
        // Файл с таким же содержимым как file1 и file2 (кросс-директорный дубликат)
        CreateFile(dir2 / "file4.txt", "This is duplicate file content. Lorem ipsum dolor sit amet."); // Дубликат
        
        // Другой набор дубликатов
        CreateFile(dir2 / "image1.jpg", "PNG image data");
        CreateFile(dir2 / "image2.jpg", "PNG image data"); // Дубликат image1
        CreateFile(dir2 / "image3.jpg", "JPG image data"); // Уникальный
        
        // Маленький файл (должен быть отфильтрован по min-size)
        CreateFile(dir2 / "small.txt", "tiny");
        
        // Пустая директория
        fs::create_directories(test_root / "empty_dir");
        
        // Вложенная директория
        fs::path nested = test_root / "nested" / "deep";
        fs::create_directories(nested);
        CreateFile(nested / "deep_file.txt", "Deep file content");
    }
    
    void CreateFile(const fs::path& path, const std::string& content) {
        std::ofstream file(path.string());
        file.write(content.data(), content.size());
    }
    
    std::vector<std::string> CreateArgv(const std::vector<std::string>& args) {
        return args;
    }
    
    fs::path test_root;
};

TEST_F(IntegrationTest, CompleteWorkflowWithDefaultSettings) {
    // Тестируем полный workflow с настройками по умолчанию
    
    // Создаем аргументы командной строки
    std::vector<std::string> args = {
        "./bayan",
        "--include", test_root.string(),
        "--min-size", "2",
        "--depth", "1"
    };
    
    // Эмулируем argc/argv
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    // Парсим конфигурацию
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    EXPECT_TRUE(config.Validate());
    EXPECT_EQ(config.include_dirs.size(), 1);
    EXPECT_EQ(config.include_dirs[0], test_root.string());
    
    // Сканируем директории
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    // Должны найти файлы
    EXPECT_FALSE(files_by_size.empty());
    
    // Создаем DuplicateFinder
    auto hasher = std::make_unique<Hasher>(config.hash_type);
    auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
    DuplicateFinder finder(std::move(cache));
    
    // Ищем дубликаты
    auto duplicates = finder.Find(files_by_size);
    
    // Должны найти хотя бы одну группу дубликатов
    // (file1.txt, file2.txt, file4.txt одинаковые)
    bool found_text_duplicates = false;
    bool found_image_duplicates = false;
    
    for (const auto& group : duplicates) {
        if (group.size() >= 2) {
            // Проверяем какие файлы в группе
            int text_count = 0;
            int image_count = 0;
            
            for (const auto& file : group) {
                if (file.find("file") != std::string::npos && 
                    file.find(".txt") != std::string::npos) {
                    text_count++;
                } else if (file.find("image") != std::string::npos && 
                          file.find(".jpg") != std::string::npos) {
                    image_count++;
                }
            }
            
            if (text_count >= 2) {
                found_text_duplicates = true;
            }
            if (image_count >= 2) {
                found_image_duplicates = true;
            }
        }
    }
    
    // Должны найти хотя бы одну группу дубликатов
    EXPECT_TRUE(found_text_duplicates || found_image_duplicates);
}

TEST_F(IntegrationTest, WorkflowWithMasks) {
    // Тестируем с фильтрацией по маскам
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", test_root.string(),
        "--depth", "1",
        "--mask", "*.txt",
        "--min-size", "2"
    };
    
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    // Должны найти только .txt файлы
    bool found_txt = false;
    bool found_jpg = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            if (file.find(".txt") != std::string::npos) {
                found_txt = true;
            }
            if (file.find(".jpg") != std::string::npos) {
                found_jpg = true;
            }
        }
    }
    
    EXPECT_TRUE(found_txt);
    EXPECT_FALSE(found_jpg); // .jpg файлы должны быть отфильтрованы
}

TEST_F(IntegrationTest, WorkflowWithExclude) {
    // Тестируем с исключением директорий
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", test_root.string(),
        "--exclude", (test_root / "dir2").string(),
        "--min-size", "1"
    };
    
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    // Не должно быть файлов из dir2
    bool found_excluded = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            if (file.find("dir2") != std::string::npos) {
                found_excluded = true;
                break;
            }
        }
        if (found_excluded) break;
    }
    
    EXPECT_FALSE(found_excluded);
}

TEST_F(IntegrationTest, WorkflowWithMinSize) {
    // Тестируем с минимальным размером файла
    
    // Определяем размер small.txt
    fs::path small_file = test_root / "dir2" / "small.txt";
    uintmax_t small_size = fs::file_size(small_file);
    
    // Устанавливаем min-size больше размера small.txt
    std::vector<std::string> args = {
        "./bayan",
        "--include", test_root.string(),
        "--min-size", std::to_string(small_size + 10)
    };
    
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    // small.txt не должен быть найден
    bool found_small = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            if (file.find("small.txt") != std::string::npos) {
                found_small = true;
                break;
            }
        }
        if (found_small) break;
    }
    
    EXPECT_FALSE(found_small);
}

TEST_F(IntegrationTest, WorkflowWithDepth) {
    // Тестируем с ограничением глубины
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", test_root.string(),
        "--depth", "1",
        "--min-size", "1"
    };
    
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    // deep_file.txt не должен быть найден (находится на глубине 2)
    bool found_deep = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            if (file.find("deep_file.txt") != std::string::npos) {
                found_deep = true;
                break;
            }
        }
        if (found_deep) break;
    }
    
    EXPECT_FALSE(found_deep);
}

TEST_F(IntegrationTest, WorkflowWithDifferentHashTypes) {
    // Тестируем с разными типами хэширования
  
    // CRC32
    {
        std::vector<std::string> args = {
            "./bayan",
            "--include", (test_root / "dir1").string(),
            "--hash", "crc32",
            "--min-size", "1"
        };
        
        int argc = args.size();
        std::vector<const char*> argv;
        for (const auto& arg : args) {
            argv.push_back(arg.c_str());
        }
        
        Parser parser;
        Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
        EXPECT_EQ(config.hash_type, HashType::CRC32);
        
        Scanner scanner(config);
        auto files_by_size = scanner.Scan();
        
        auto hasher = std::make_unique<Hasher>(config.hash_type);
        auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
        DuplicateFinder finder(std::move(cache));
        
        auto duplicates = finder.Find(files_by_size);
        
        // Должны найти дубликаты
        bool found_duplicates = false;
        for (const auto& group : duplicates) {
            if (group.size() >= 2) {
                found_duplicates = true;
                break;
            }
        }
        EXPECT_TRUE(found_duplicates);
    }
    
    // MD5
    {
        std::vector<std::string> args = {
            "./bayan",
            "--include", (test_root / "dir1").string(),
            "--hash", "md5",
            "--min-size", "1"
        };
        
        int argc = args.size();
        std::vector<const char*> argv;
        for (const auto& arg : args) {
            argv.push_back(arg.c_str());
        }
        
        Parser parser;
        Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
        EXPECT_EQ(config.hash_type, HashType::MD5);
        
        Scanner scanner(config);
        auto files_by_size = scanner.Scan();
        
        auto hasher = std::make_unique<Hasher>(config.hash_type);
        auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
        DuplicateFinder finder(std::move(cache));
        
        auto duplicates = finder.Find(files_by_size);
        
        // Должны найти дубликаты
        bool found_duplicates = false;
        for (const auto& group : duplicates) {
            if (group.size() >= 2) {
                found_duplicates = true;
                break;
            }
        }
        EXPECT_TRUE(found_duplicates);
    }
}

TEST_F(IntegrationTest, WorkflowWithDifferentBlockSizes) {
    // Тестируем с разными размерами блоков
    
    // Маленький блок
    {
        std::vector<std::string> args = {
            "./bayan",
            "--include", (test_root / "dir1").string(),
            "--block", "512",
            "--min-size", "1"
        };
        
        int argc = args.size();
        std::vector<const char*> argv;
        for (const auto& arg : args) {
            argv.push_back(arg.c_str());
        }
        
        Parser parser;
        Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
        EXPECT_EQ(config.block_size, 512);
        
        Scanner scanner(config);
        auto files_by_size = scanner.Scan();
        
        auto hasher = std::make_unique<Hasher>(config.hash_type);
        auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
        DuplicateFinder finder(std::move(cache));
        
        auto duplicates = finder.Find(files_by_size);
        
        // Должны найти дубликаты
        bool found_duplicates = false;
        for (const auto& group : duplicates) {
            if (group.size() >= 2) {
                found_duplicates = true;
                break;
            }
        }
        EXPECT_TRUE(found_duplicates);
    }
    
    // Большой блок
    {
        std::vector<std::string> args = {
            "./bayan",
            "--include", (test_root / "dir1").string(),
            "--block", "16384",
            "--min-size", "1"
        };
        
        int argc = args.size();
        std::vector<const char*> argv;
        for (const auto& arg : args) {
            argv.push_back(arg.c_str());
        }
        
        Parser parser;
        Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
        EXPECT_EQ(config.block_size, 16384);
        
        Scanner scanner(config);
        auto files_by_size = scanner.Scan();
        
        auto hasher = std::make_unique<Hasher>(config.hash_type);
        auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
        DuplicateFinder finder(std::move(cache));
        
        auto duplicates = finder.Find(files_by_size);
        
        // Должны найти дубликаты
        bool found_duplicates = false;
        for (const auto& group : duplicates) {
            if (group.size() >= 2) {
                found_duplicates = true;
                break;
            }
        }
        EXPECT_TRUE(found_duplicates);
    }
}

TEST_F(IntegrationTest, NoDuplicatesFound) {
    // Тестируем случай когда дубликатов нет
    
    // Создаем директорию с уникальными файлами
    fs::path unique_dir = test_root / "unique";
    fs::create_directories(unique_dir);
    
    CreateFile(unique_dir / "a.txt", "Unique content A");
    CreateFile(unique_dir / "b.txt", "Unique content B");
    CreateFile(unique_dir / "c.txt", "Unique content C");
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", unique_dir.string(),
        "--min-size", "1"
    };
    
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    // Должны найти файлы
    EXPECT_FALSE(files_by_size.empty());
    
    auto hasher = std::make_unique<Hasher>(config.hash_type);
    auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
    DuplicateFinder finder(std::move(cache));
    
    auto duplicates = finder.Find(files_by_size);
    
    // Не должно быть дубликатов
    EXPECT_TRUE(duplicates.empty());
}

TEST_F(IntegrationTest, EmptyDirectory) {
    // Тестируем пустую директорию
    
    fs::path empty_dir = test_root / "really_empty";
    fs::create_directories(empty_dir);
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", empty_dir.string(),
        "--min-size", "1"
    };
    
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    // Не должно быть файлов
    EXPECT_TRUE(files_by_size.empty());
    
    auto hasher = std::make_unique<Hasher>(config.hash_type);
    auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
    DuplicateFinder finder(std::move(cache));
    
    auto duplicates = finder.Find(files_by_size);
    
    // Не должно быть дубликатов
    EXPECT_TRUE(duplicates.empty());
}

TEST_F(IntegrationTest, MultipleIncludeDirectories) {
    // Тестируем несколько включаемых директорий
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", (test_root / "dir1").string(),
        "--include", (test_root / "dir2").string(),
        "--min-size", "1"
    };
    
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    EXPECT_EQ(config.include_dirs.size(), 2);
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    // Должны найти файлы из обеих директорий
    bool found_dir1 = false;
    bool found_dir2 = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            if (file.find("dir1") != std::string::npos) {
                found_dir1 = true;
            }
            if (file.find("dir2") != std::string::npos) {
                found_dir2 = true;
            }
        }
    }
    
    EXPECT_TRUE(found_dir1);
    EXPECT_TRUE(found_dir2);
}

TEST_F(IntegrationTest, ComplexConfiguration) {
    // Тестируем сложную конфигурацию со всеми параметрами
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", test_root.string(),
        "--exclude", (test_root / "dir2").string(),
        "--depth", "2",
        "--min-size", "10",
        "--mask", "*.txt",
        "--block", "1024",
        "--hash", "md5"
    };
    
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    EXPECT_TRUE(config.Validate());
    EXPECT_EQ(config.include_dirs.size(), 1);
    EXPECT_EQ(config.exclude_dirs.size(), 1);
    EXPECT_EQ(config.depth, 2);
    EXPECT_EQ(config.min_file_size, 10);
    EXPECT_EQ(config.masks.size(), 1);
    EXPECT_EQ(config.masks[0], "*.txt");
    EXPECT_EQ(config.block_size, 1024);
    EXPECT_EQ(config.hash_type, HashType::MD5);
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    auto hasher = std::make_unique<Hasher>(config.hash_type);
    auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
    DuplicateFinder finder(std::move(cache));
    
    auto duplicates = finder.Find(files_by_size);
    
    // Просто проверяем что не падает
    EXPECT_NO_THROW();
}

TEST_F(IntegrationTest, FileModificationDoesNotAffectHashing) {
    // Тестируем что изменение времени модификации не влияет на определение дубликатов
    
    fs::path dir = test_root / "mod_test";
    fs::create_directories(dir);
    
    // Создаем два одинаковых файла
    fs::path file1 = dir / "original.txt";
    fs::path file2 = dir / "copy.txt";
    
    std::string content = "Same content for both files";
    CreateFile(file1, content);
    CreateFile(file2, content);
    
    // Меняем время модификации второго файла
    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::ofstream file(file2.string(), std::ios::app);
        file << " "; // Добавляем пробел и удаляем его
    }
    
    // Удаляем пробел чтобы содержимое осталось прежним
    std::string file2_content;
    {
        std::ifstream file(file2.string());
        std::stringstream buffer;
        buffer << file.rdbuf();
        file2_content = buffer.str();
    }
    
    // Удаляем последний символ если это пробел
    if (!file2_content.empty() && file2_content.back() == ' ') {
        file2_content.pop_back();
        std::ofstream file(file2.string());
        file << file2_content;
    }
    
    // Теперь файлы имеют одинаковое содержимое но разное время модификации
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", dir.string(),
        "--min-size", "1"
    };
    
    int argc = args.size();
    std::vector<const char*> argv;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
    }
    
    Parser parser;
    Config config = parser.Parse(argc, const_cast<char**>(argv.data()));
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();
    
    auto hasher = std::make_unique<Hasher>(config.hash_type);
    auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
    DuplicateFinder finder(std::move(cache));
    
    auto duplicates = finder.Find(files_by_size);
    
    // Должны найти дубликаты (файлы одинаковые несмотря на разное время)
    bool found_duplicates = false;
    for (const auto& group : duplicates) {
        if (group.size() >= 2) {
            bool has_file1 = false;
            bool has_file2 = false;
            
            for (const auto& file : group) {
                if (file.find("original.txt") != std::string::npos) {
                    has_file1 = true;
                }
                if (file.find("copy.txt") != std::string::npos) {
                    has_file2 = true;
                }
            }
            
            if (has_file1 && has_file2) {
                found_duplicates = true;
                break;
            }
        }
    }
    
    EXPECT_TRUE(found_duplicates);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}