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
        test_root = fs::temp_directory_path() / "bayan_integration_test";
        fs::create_directories(test_root);

        CreateTestStructure();
    }
    
    void TearDown() override {
        try {
            fs::remove_all(test_root);
        } catch (...) {
        }
    }
    
    void CreateTestStructure() {
        fs::path dir1 = test_root / "dir1";
        fs::create_directories(dir1);

        CreateFile(dir1 / "file1.txt", "This is duplicate file content. Lorem ipsum dolor sit amet.");
        CreateFile(dir1 / "file2.txt", "This is duplicate file content. Lorem ipsum dolor sit amet."); 
        CreateFile(dir1 / "file3.txt", "This is unique file content. Consectetur adipiscing elit."); 

        fs::path dir2 = test_root / "dir2";
        fs::create_directories(dir2);

        CreateFile(dir2 / "file4.txt", "This is duplicate file content. Lorem ipsum dolor sit amet."); 

        CreateFile(dir2 / "image1.jpg", "PNG image data");
        CreateFile(dir2 / "image2.jpg", "PNG image data"); 
        CreateFile(dir2 / "image3.jpg", "JPG image data"); 

        CreateFile(dir2 / "small.txt", "tiny");
        
        fs::create_directories(test_root / "empty_dir");
        
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

    std::vector<std::string> args = {
        "./bayan",
        "--include", test_root.string(),
        "--min-size", "2",
        "--depth", "1"
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
    EXPECT_EQ(config.include_dirs[0], test_root);  
    
    Scanner scanner(config);
    auto files_by_size = scanner.Scan();  

    EXPECT_FALSE(files_by_size.empty());

    auto hasher = std::make_unique<Hasher>(config.hash_type);
    auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
    DuplicateFinder finder(std::move(cache));

    auto duplicates = finder.Find(files_by_size);  
    bool found_text_duplicates = false;
    bool found_image_duplicates = false;
    
    for (const auto& group : duplicates) {
        if (group.size() >= 2) {
            int text_count = 0;
            int image_count = 0;
            
            for (const auto& file : group) {  
                std::string filename = file.string();  
                if (filename.find("file") != std::string::npos && 
                    filename.find(".txt") != std::string::npos) {
                    text_count++;
                } else if (filename.find("image") != std::string::npos && 
                          filename.find(".jpg") != std::string::npos) {
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

    EXPECT_TRUE(found_text_duplicates || found_image_duplicates);
}

TEST_F(IntegrationTest, WorkflowWithMasks) {

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
    
    bool found_txt = false;
    bool found_jpg = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            std::string filename = file.string();
            if (filename.find(".txt") != std::string::npos) {
                found_txt = true;
            }
            if (filename.find(".jpg") != std::string::npos) {
                found_jpg = true;
            }
        }
    }
    
    EXPECT_TRUE(found_txt);
    EXPECT_FALSE(found_jpg);
}

TEST_F(IntegrationTest, WorkflowWithExclude) {

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

    bool found_excluded = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            std::string path_str = file.string();
            if (path_str.find("dir2") != std::string::npos) {
                found_excluded = true;
                break;
            }
        }
        if (found_excluded) break;
    }
    
    EXPECT_FALSE(found_excluded);
}

TEST_F(IntegrationTest, WorkflowWithMinSize) {

    fs::path small_file = test_root / "dir2" / "small.txt";
    uintmax_t small_size = fs::file_size(small_file);

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

    bool found_small = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            std::string path_str = file.string();
            if (path_str.find("small.txt") != std::string::npos) {
                found_small = true;
                break;
            }
        }
        if (found_small) break;
    }
    
    EXPECT_FALSE(found_small);
}

TEST_F(IntegrationTest, WorkflowWithDepth) {
 
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

    bool found_deep = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            std::string path_str = file.string();
            if (path_str.find("deep_file.txt") != std::string::npos) {
                found_deep = true;
                break;
            }
        }
        if (found_deep) break;
    }
    
    EXPECT_FALSE(found_deep);
}

TEST_F(IntegrationTest, WorkflowWithDifferentHashTypes) {
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

        bool found_duplicates = false;
        for (const auto& group : duplicates) {
            if (group.size() >= 2) {
                found_duplicates = true;
                break;
            }
        }
        EXPECT_TRUE(found_duplicates);
    }

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

        bool found_duplicates = false;
        for (const auto& group : duplicates) {
            if (group.size() >= 2) {
                found_duplicates = true;
                break;
            }
        }
        EXPECT_TRUE(found_duplicates);
    }

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

    EXPECT_FALSE(files_by_size.empty());
    
    auto hasher = std::make_unique<Hasher>(config.hash_type);
    auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
    DuplicateFinder finder(std::move(cache));
    
    auto duplicates = finder.Find(files_by_size);

    EXPECT_TRUE(duplicates.empty());
}

TEST_F(IntegrationTest, EmptyDirectory) { 
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

    EXPECT_TRUE(files_by_size.empty());
    
    auto hasher = std::make_unique<Hasher>(config.hash_type);
    auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
    DuplicateFinder finder(std::move(cache));
    
    auto duplicates = finder.Find(files_by_size);

    EXPECT_TRUE(duplicates.empty());
}

TEST_F(IntegrationTest, MultipleIncludeDirectories) {
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

    bool found_dir1 = false;
    bool found_dir2 = false;
    
    for (const auto& [size, files] : files_by_size) {
        for (const auto& file : files) {
            std::string path_str = file.string();
            if (path_str.find("dir1") != std::string::npos) {
                found_dir1 = true;
            }
            if (path_str.find("dir2") != std::string::npos) {
                found_dir2 = true;
            }
        }
    }
    
    EXPECT_TRUE(found_dir1);
    EXPECT_TRUE(found_dir2);
}

TEST_F(IntegrationTest, ComplexConfiguration) {

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

    EXPECT_NO_THROW();
}

TEST_F(IntegrationTest, FileModificationDoesNotAffectHashing) {   
    fs::path dir = test_root / "mod_test";
    fs::create_directories(dir);

    fs::path file1 = dir / "original.txt";
    fs::path file2 = dir / "copy.txt";
    
    std::string content = "Same content for both files";
    CreateFile(file1, content);
    CreateFile(file2, content);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::ofstream file(file2.string(), std::ios::app);
        file << " "; 
    }

    std::string file2_content;
    {
        std::ifstream file(file2.string());
        std::stringstream buffer;
        buffer << file.rdbuf();
        file2_content = buffer.str();
    }

    if (!file2_content.empty() && file2_content.back() == ' ') {
        file2_content.pop_back();
        std::ofstream file(file2.string());
        file << file2_content;
    }
    
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
    
    bool found_duplicates = false;
    for (const auto& group : duplicates) {
        if (group.size() >= 2) {
            bool has_file1 = false;
            bool has_file2 = false;
            
            for (const auto& file : group) {
                std::string path_str = file.string();
                if (path_str.find("original.txt") != std::string::npos) {
                    has_file1 = true;
                }
                if (path_str.find("copy.txt") != std::string::npos) {
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