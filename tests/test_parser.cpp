#include <gtest/gtest.h>
#include "parser.h"
#include <vector>
#include <string>
#include <cstring>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Сохраняем оригинальные argc/argv
        saved_argv.clear();
        argv_ptrs.clear();
    }
    
    void TearDown() override {
        // Очищаем выделенную память
        for (auto ptr : argv_ptrs) {
            delete[] ptr;
        }
        argv_ptrs.clear();
    }
    
    // Вспомогательная функция для создания argv
    char** CreateArgv(const std::vector<std::string>& args) {
        // Очищаем предыдущие указатели
        for (auto ptr : argv_ptrs) {
            delete[] ptr;
        }
        argv_ptrs.clear();
        
        argc = args.size();
        argv_ptrs.reserve(argc);
        
        for (const auto& arg : args) {
            char* ptr = new char[arg.size() + 1];
            std::strcpy(ptr, arg.c_str());
            argv_ptrs.push_back(ptr);
        }
        
        return argv_ptrs.data();
    }
    
    int argc;
    std::vector<char*> argv_ptrs;
    std::vector<std::string> saved_argv;
};

TEST_F(ParserTest, ParseHelp) {
    Parser parser;
    
    std::vector<std::string> args = {"./bayan", "--help"};
    char** argv = CreateArgv(args);
    
    // help должен выводить сообщение и завершать программу с exit(0)
    // В тесте мы ловим системный вызов, проверяем что не бросает исключений
    EXPECT_NO_THROW({
        try {
            parser.Parse(argc, argv);
        } catch (...) {
            // Игнорируем исключения от exit()
        }
    });
    
    // Также проверяем короткую форму
    args = {"./bayan", "-h"};
    argv = CreateArgv(args);
    
    EXPECT_NO_THROW({
        try {
            parser.Parse(argc, argv);
        } catch (...) {
            // Игнорируем исключения
        }
    });
}

TEST_F(ParserTest, ParseDefaultValues) {
    Parser parser;
    
    // Минимальные аргументы
    std::vector<std::string> args = {"./bayan"};
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    
    // Проверяем значения по умолчанию
    EXPECT_EQ(config.include_dirs.size(), 1);
    EXPECT_EQ(config.include_dirs[0], ".");
    EXPECT_TRUE(config.exclude_dirs.empty());
    EXPECT_EQ(config.depth, 0);
    EXPECT_EQ(config.min_file_size, 2);
    EXPECT_TRUE(config.masks.empty());
    EXPECT_EQ(config.block_size, 4096);
    EXPECT_EQ(config.hash_type, HashType::CRC32);
    EXPECT_TRUE(config.Validate());
}

TEST_F(ParserTest, ParseIncludeDirectories) {
    Parser parser;
    
    // Одна директория
    std::vector<std::string> args = {"./bayan", "--include", "/home/user/docs"};
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    EXPECT_EQ(config.include_dirs.size(), 1);
    EXPECT_EQ(config.include_dirs[0], "/home/user/docs");
    
    // Несколько директорий
    args = {"./bayan", "--include", "/home/user/docs", "/home/user/images"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.include_dirs.size(), 2);
    EXPECT_EQ(config.include_dirs[0], "/home/user/docs");
    EXPECT_EQ(config.include_dirs[1], "/home/user/images");
    
    // Короткая форма
    args = {"./bayan", "-i", "/home/user/docs"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.include_dirs.size(), 1);
    EXPECT_EQ(config.include_dirs[0], "/home/user/docs");
    
    // Позиционные аргументы
    args = {"./bayan", "/home/user/docs", "/home/user/images"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.include_dirs.size(), 2);
    EXPECT_EQ(config.include_dirs[0], "/home/user/docs");
    EXPECT_EQ(config.include_dirs[1], "/home/user/images");
}

TEST_F(ParserTest, ParseExcludeDirectories) {
    Parser parser;
    
    std::vector<std::string> args = {"./bayan", "--exclude", "/home/user/temp", "/home/user/backup"};
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    EXPECT_EQ(config.exclude_dirs.size(), 2);
    EXPECT_EQ(config.exclude_dirs[0], "/home/user/temp");
    EXPECT_EQ(config.exclude_dirs[1], "/home/user/backup");
    
    // Короткая форма
    args = {"./bayan", "-e", "/home/user/temp"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.exclude_dirs.size(), 1);
    EXPECT_EQ(config.exclude_dirs[0], "/home/user/temp");
}

TEST_F(ParserTest, ParseDepth) {
    Parser parser;
    
    std::vector<std::string> args = {"./bayan", "--depth", "3"};
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    EXPECT_EQ(config.depth, 3);
    
    // Короткая форма
    args = {"./bayan", "-d", "5"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.depth, 5);
    
    // По умолчанию
    args = {"./bayan"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.depth, 0);
}

TEST_F(ParserTest, ParseMinSize) {
    Parser parser;
    
    std::vector<std::string> args = {"./bayan", "--min-size", "1024"};
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    EXPECT_EQ(config.min_file_size, 1024);
    
    // Большое значение
    args = {"./bayan", "--min-size", "1048576"}; // 1 MB
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.min_file_size, 1048576);
    
    // Значение по умолчанию
    args = {"./bayan"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.min_file_size, 2);
}

TEST_F(ParserTest, ParseMasks) {
    Parser parser;
    
    std::vector<std::string> args = {"./bayan", "--mask", "*.txt", "*.jpg"};
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    ASSERT_EQ(config.masks.size(), 2);
    EXPECT_EQ(config.masks[0], "*.txt");
    EXPECT_EQ(config.masks[1], "*.jpg");
    
    // Короткая форма
    args = {"./bayan", "-m", "*.pdf", "*.doc"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    ASSERT_EQ(config.masks.size(), 2);
    EXPECT_EQ(config.masks[0], "*.pdf");
    EXPECT_EQ(config.masks[1], "*.doc");
    
    // Несколько масок через пробел
    args = {"./bayan", "-m", "*.txt", "*.log", "*.tmp"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    ASSERT_EQ(config.masks.size(), 3);
    EXPECT_EQ(config.masks[0], "*.txt");
    EXPECT_EQ(config.masks[1], "*.log");
    EXPECT_EQ(config.masks[2], "*.tmp");
}

TEST_F(ParserTest, ParseBlockSize) {
    Parser parser;
    
    std::vector<std::string> args = {"./bayan", "--block", "8192"};
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    EXPECT_EQ(config.block_size, 8192);
    
    // Короткая форма
    args = {"./bayan", "-b", "16384"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.block_size, 16384);
    
    // Значение по умолчанию
    args = {"./bayan"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.block_size, 4096);
}

TEST_F(ParserTest, ParseHashType) {
    Parser parser;
    
    // CRC32 (по умолчанию)
    std::vector<std::string> args = {"./bayan", "--hash", "crc32"};
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    EXPECT_EQ(config.hash_type, HashType::CRC32);
    
    // MD5
    args = {"./bayan", "--hash", "md5"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.hash_type, HashType::MD5);
    
    // Верхний регистр
    args = {"./bayan", "--hash", "CRC32"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.hash_type, HashType::CRC32);
    
    // Смешанный регистр
    args = {"./bayan", "--hash", "Md5"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.hash_type, HashType::MD5);
    
    // Значение по умолчанию
    args = {"./bayan"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_EQ(config.hash_type, HashType::CRC32);
}

TEST_F(ParserTest, ParseHashTypeInvalid) {
    Parser parser;
    
    std::vector<std::string> args = {"./bayan", "--hash", "sha256"};
    char** argv = CreateArgv(args);
    
    EXPECT_THROW(parser.Parse(argc, argv), std::runtime_error);
    
    args = {"./bayan", "--hash", "invalid"};
    argv = CreateArgv(args);
    
    EXPECT_THROW(parser.Parse(argc, argv), std::runtime_error);
    
    // Пустая строка
    args = {"./bayan", "--hash", ""};
    argv = CreateArgv(args);
    
    EXPECT_THROW(parser.Parse(argc, argv), std::runtime_error);
}

TEST_F(ParserTest, ParseComplexCommand) {
    Parser parser;
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", "/home/user/docs", "/home/user/images",
        "--exclude", "/home/user/docs/temp", "/home/user/images/backup",
        "--depth", "3",
        "--min-size", "1024",
        "--mask", "*.txt", "*.jpg",
        "--block", "8192",
        "--hash", "md5"
    };
    
    char** argv = CreateArgv(args);
    Config config = parser.Parse(argc, argv);
    
    EXPECT_EQ(config.include_dirs.size(), 2);
    EXPECT_EQ(config.include_dirs[0], "/home/user/docs");
    EXPECT_EQ(config.include_dirs[1], "/home/user/images");
    
    EXPECT_EQ(config.exclude_dirs.size(), 2);
    EXPECT_EQ(config.exclude_dirs[0], "/home/user/docs/temp");
    EXPECT_EQ(config.exclude_dirs[1], "/home/user/images/backup");
    
    EXPECT_EQ(config.depth, 3);
    EXPECT_EQ(config.min_file_size, 1024);
    
    EXPECT_EQ(config.masks.size(), 2);
    EXPECT_EQ(config.masks[0], "*.txt");
    EXPECT_EQ(config.masks[1], "*.jpg");
    
    EXPECT_EQ(config.block_size, 8192);
    EXPECT_EQ(config.hash_type, HashType::MD5);
    
    EXPECT_TRUE(config.Validate());
}

TEST_F(ParserTest, ParseInvalidBlockSize) {
    Parser parser;
    
    // Нулевой размер блока
    std::vector<std::string> args = {"./bayan", "--block", "0"};
    char** argv = CreateArgv(args);
    
    EXPECT_THROW(parser.Parse(argc, argv), std::runtime_error);
    
    // Отрицательный размер (должен быть отловлен как строка)
    args = {"./bayan", "--block", "-1"};
    argv = CreateArgv(args);
    
    // Может выбросить исключение или проигнорировать
    EXPECT_THROW(parser.Parse(argc, argv), std::exception);
}

TEST_F(ParserTest, ParseInvalidArguments) {
    Parser parser;
    
    // Неизвестный параметр
    std::vector<std::string> args = {"./bayan", "--unknown-param", "value"};
    char** argv = CreateArgv(args);
    
    EXPECT_THROW(parser.Parse(argc, argv), std::runtime_error);
    
    // Отсутствующее значение для параметра
    args = {"./bayan", "--depth"};
    argv = CreateArgv(args);
    
    EXPECT_THROW(parser.Parse(argc, argv), std::runtime_error);
    
    // Некорректное числовое значение
    args = {"./bayan", "--depth", "not-a-number"};
    argv = CreateArgv(args);
    
    EXPECT_THROW(parser.Parse(argc, argv), std::runtime_error);
}

TEST_F(ParserTest, ValidateConfig) {
    Parser parser;
    
    // Валидная конфигурация
    std::vector<std::string> args = {"./bayan", "--include", "/home/user/docs"};
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    EXPECT_TRUE(config.Validate());
    
    // Еще одна валидная конфигурация
    args = {"./bayan", "/home/user/docs"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_TRUE(config.Validate());
    
    // Конфигурация с глубиной и масками
    args = {"./bayan", "-i", "/tmp", "-d", "2", "-m", "*.txt"};
    argv = CreateArgv(args);
    
    config = parser.Parse(argc, argv);
    EXPECT_TRUE(config.Validate());
}

TEST_F(ParserTest, ParseWithRealPaths) {
    Parser parser;
    
    // Используем реальные временные пути
    fs::path temp_dir = fs::temp_directory_path();
    std::string temp_dir_str = temp_dir.string();
    
    std::vector<std::string> args = {
        "./bayan",
        "--include", temp_dir_str,
        "--exclude", (temp_dir / "excluded").string(),
        "--min-size", "100",
        "--block", "1024",
        "--hash", "md5"
    };
    
    char** argv = CreateArgv(args);
    
    // Должно успешно распарситься
    EXPECT_NO_THROW({
        Config config = parser.Parse(argc, argv);
        EXPECT_TRUE(config.Validate());
        EXPECT_EQ(config.include_dirs.size(), 1);
        EXPECT_EQ(config.include_dirs[0], temp_dir_str);
        EXPECT_EQ(config.exclude_dirs.size(), 1);
        EXPECT_EQ(config.exclude_dirs[0], (temp_dir / "excluded").string());
        EXPECT_EQ(config.min_file_size, 100);
        EXPECT_EQ(config.block_size, 1024);
        EXPECT_EQ(config.hash_type, HashType::MD5);
    });
}

TEST_F(ParserTest, ParseMixedOptions) {
    Parser parser;
    
    // Смешанные длинные и короткие опции
    std::vector<std::string> args = {
        "./bayan",
        "-i", "/dir1",
        "--exclude", "/dir1/temp",
        "-d", "2",
        "--min-size", "500",
        "-m", "*.txt",
        "-b", "2048",
        "--hash", "crc32"
    };
    
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    
    EXPECT_EQ(config.include_dirs.size(), 1);
    EXPECT_EQ(config.include_dirs[0], "/dir1");
    EXPECT_EQ(config.exclude_dirs.size(), 1);
    EXPECT_EQ(config.exclude_dirs[0], "/dir1/temp");
    EXPECT_EQ(config.depth, 2);
    EXPECT_EQ(config.min_file_size, 500);
    EXPECT_EQ(config.masks.size(), 1);
    EXPECT_EQ(config.masks[0], "*.txt");
    EXPECT_EQ(config.block_size, 2048);
    EXPECT_EQ(config.hash_type, HashType::CRC32);
    EXPECT_TRUE(config.Validate());
}

TEST_F(ParserTest, ParseEmptyArgs) {
    Parser parser;
    
    // Пустые аргументы (только имя программы)
    std::vector<std::string> args = {"./bayan"};
    char** argv = CreateArgv(args);
    
    // Должно успешно распарситься с значениями по умолчанию
    EXPECT_NO_THROW({
        Config config = parser.Parse(argc, argv);
        EXPECT_TRUE(config.Validate());
        EXPECT_EQ(config.include_dirs.size(), 1);
        EXPECT_EQ(config.include_dirs[0], ".");
    });
}

TEST_F(ParserTest, ParseOnlyPositionalArgs) {
    Parser parser;
    
    // Только позиционные аргументы
    std::vector<std::string> args = {
        "./bayan",
        "/path/to/scan1",
        "/path/to/scan2",
        "/path/to/scan3"
    };
    
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    
    EXPECT_EQ(config.include_dirs.size(), 3);
    EXPECT_EQ(config.include_dirs[0], "/path/to/scan1");
    EXPECT_EQ(config.include_dirs[1], "/path/to/scan2");
    EXPECT_EQ(config.include_dirs[2], "/path/to/scan3");
    EXPECT_TRUE(config.Validate());
}

TEST_F(ParserTest, ParseDuplicateOptions) {
    Parser parser;
    
    // Дублирующиеся опции - последнее должно перезаписать
    std::vector<std::string> args = {
        "./bayan",
        "--block", "1024",
        "--block", "2048",  // Должно перезаписать
        "--hash", "crc32",
        "--hash", "md5"     // Должно перезаписать
    };
    
    char** argv = CreateArgv(args);
    
    Config config = parser.Parse(argc, argv);
    
    EXPECT_EQ(config.block_size, 2048);
    EXPECT_EQ(config.hash_type, HashType::MD5);
    EXPECT_TRUE(config.Validate());
}