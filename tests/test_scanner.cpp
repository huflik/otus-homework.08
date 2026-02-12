#include <gtest/gtest.h>
#include "scanner.h"
#include "config.h"
#include <boost/filesystem.hpp>
#include <fstream>

namespace fs = boost::filesystem;

class ScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_root = fs::temp_directory_path() / "scanner_test";
        fs::create_directories(test_root);

        CreateTestFiles();
    }
    
    void TearDown() override {
        try {
            fs::remove_all(test_root);
        } catch (...) {
        }
    }
    
    void CreateTestFiles() {

        CreateFile("small.txt", 50);      
        CreateFile("medium.txt", 2000);   
        CreateFile("large.txt", 5000);   

        CreateFile("document.pdf", 3000);
        CreateFile("image.jpg", 4000);
        CreateFile("text.txt", 3000);

        fs::create_directories(test_root / "subdir1");
        fs::create_directories(test_root / "subdir2");
        
        CreateFile("subdir1/file1.txt", 3000);
        CreateFile("subdir1/file2.txt", 3000);
        CreateFile("subdir2/file3.txt", 3000);
    }
    
    void CreateFile(const std::string& relative_path, size_t size) {
        fs::path file_path = test_root / relative_path;
        std::ofstream file(file_path.string(), std::ios::binary);

        for (size_t i = 0; i < size; ++i) {
            file.put(static_cast<char>(i % 256));
        }
    }
    
    fs::path test_root;
};

TEST_F(ScannerTest, ScanBasic) {
    Config config;
    config.include_dirs.push_back(test_root);
    config.min_file_size = 100; 
    
    Scanner scanner(config);
    auto result = scanner.Scan();

    EXPECT_FALSE(result.empty());

    bool found_medium = false;
    bool found_large = false;
    
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            std::string path_str = file.string();
            if (path_str.find("medium.txt") != std::string::npos) {
                found_medium = true;
            }
            if (path_str.find("large.txt") != std::string::npos) {
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
    config.min_file_size = 3000; 
    
    Scanner scanner(config);
    auto result = scanner.Scan();

    for (const auto& [size, files] : result) {
        EXPECT_GE(size, config.min_file_size);

        for (const auto& file : files) {
            std::string path_str = file.string();
            EXPECT_TRUE(path_str.find("small.txt") == std::string::npos);
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

    bool found_txt = false;
    bool found_jpg = false;
    bool found_pdf = false;
    
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            std::string path_str = file.string();
            if (path_str.find(".txt") != std::string::npos) {
                found_txt = true;
            } else if (path_str.find(".jpg") != std::string::npos) {
                found_jpg = true;
            } else if (path_str.find(".pdf") != std::string::npos) {
                found_pdf = true;
            }
        }
    }
    
    EXPECT_TRUE(found_txt);
    EXPECT_TRUE(found_jpg);
    EXPECT_FALSE(found_pdf);
}

TEST_F(ScannerTest, ScanMultipleDirectories) {
    Config config;
    config.include_dirs.push_back(test_root / "subdir1");
    config.include_dirs.push_back(test_root / "subdir2");
    config.min_file_size = 1;
    
    Scanner scanner(config);
    auto result = scanner.Scan();
    
    bool found_subdir1 = false;
    bool found_subdir2 = false;
    
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            std::string path_str = file.string();
            if (path_str.find("subdir1") != std::string::npos) {
                found_subdir1 = true;
            }
            if (path_str.find("subdir2") != std::string::npos) {
                found_subdir2 = true;
            }
        }
    }
    
    EXPECT_TRUE(found_subdir1);
    EXPECT_TRUE(found_subdir2);
}

TEST_F(ScannerTest, ScanExcludeDirectories) {
    Config config;
    config.include_dirs.push_back(test_root);
    config.min_file_size = 1;
    config.exclude_dirs.push_back(test_root / "subdir1");
    
    Scanner scanner(config);
    auto result = scanner.Scan();

    bool found_excluded = false;
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            std::string path_str = file.string();
            if (path_str.find("subdir1") != std::string::npos) {
                found_excluded = true;
                break;
            }
        }
    }
    
    EXPECT_FALSE(found_excluded);
}

TEST_F(ScannerTest, ScanEmptyDirectory) {
    fs::path empty_dir = fs::temp_directory_path() / "empty_test";
    fs::create_directories(empty_dir);
    
    Config config;
    config.include_dirs.push_back(empty_dir);
    config.min_file_size = 1;
    
    Scanner scanner(config);
    auto result = scanner.Scan();

    EXPECT_TRUE(result.empty());

    fs::remove_all(empty_dir);
}

TEST_F(ScannerTest, ScanNonExistentDirectory) {
    Config config;
    config.include_dirs.push_back("/non/existent/directory");
    config.min_file_size = 1;
    
    Scanner scanner(config);

    EXPECT_NO_THROW(scanner.Scan());
}

TEST_F(ScannerTest, ScanGroupsBySize) {
    Config config;
    config.include_dirs.push_back(test_root);
    config.min_file_size = 1;
    
    Scanner scanner(config);
    auto result = scanner.Scan();

    for (const auto& [size, files] : result) {
        EXPECT_FALSE(files.empty());

        for (const auto& file : files) {
            try {
                uintmax_t file_size = fs::file_size(file);
                EXPECT_EQ(file_size, size);
            } catch (...) {
            }
        }
    }
}

TEST_F(ScannerTest, ScanDuplicatePaths) {
    Config config;
    config.include_dirs.push_back(test_root);
    config.include_dirs.push_back(test_root);
    config.min_file_size = 1;
    
    Scanner scanner(config);
    auto result = scanner.Scan();

    std::set<std::string> unique_files;
    for (const auto& [size, files] : result) {
        for (const auto& file : files) {
            unique_files.insert(file.string());
        }
    }

    size_t total_files = 0;
    for (const auto& [size, files] : result) {
        total_files += files.size();
    }
    
    EXPECT_EQ(unique_files.size(), total_files);
}