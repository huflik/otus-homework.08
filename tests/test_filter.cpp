#include <gtest/gtest.h>
#include "filter.h"
#include <vector>
#include <string>

TEST(FilterTest, ConstructorEmptyMasks) {
    std::vector<std::string> empty_masks;
    Filter filter(empty_masks);

    EXPECT_TRUE(filter.Match("test.txt"));
    EXPECT_TRUE(filter.Match("file.jpg"));
    EXPECT_TRUE(filter.Match("document.pdf"));
}

TEST(FilterTest, ConstructorWithMasks) {
    std::vector<std::string> masks = {"*.txt", "*.jpg"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("test.txt"));
    EXPECT_TRUE(filter.Match("file.jpg"));
    EXPECT_FALSE(filter.Match("document.pdf"));
    EXPECT_FALSE(filter.Match("image.png"));
}

TEST(FilterTest, CaseInsensitive) {
    std::vector<std::string> masks = {"*.TXT", "*.JPG"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("test.txt"));
    EXPECT_TRUE(filter.Match("TEST.TXT"));
    EXPECT_TRUE(filter.Match("file.jpg"));
    EXPECT_TRUE(filter.Match("File.Jpg"));
    EXPECT_FALSE(filter.Match("document.pdf"));
}

TEST(FilterTest, MultipleMasks) {
    std::vector<std::string> masks = {"*.txt", "*.log", "file_*"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("test.txt"));
    EXPECT_TRUE(filter.Match("error.log"));
    EXPECT_TRUE(filter.Match("file_backup.dat"));
    EXPECT_TRUE(filter.Match("FILE_123.tmp"));
    EXPECT_FALSE(filter.Match("temp.dat"));
    EXPECT_FALSE(filter.Match("backup.bak"));
}

TEST(FilterTest, SimpleWildcard) {
    std::vector<std::string> masks = {"test*"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("test.txt"));
    EXPECT_TRUE(filter.Match("test123.dat"));
    EXPECT_TRUE(filter.Match("test_file.jpg"));
    EXPECT_FALSE(filter.Match("temp.txt"));
    EXPECT_FALSE(filter.Match("atest.txt"));
}

TEST(FilterTest, SimpleQuestionMark) {
    std::vector<std::string> masks = {"file?.txt"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("file1.txt"));
    EXPECT_TRUE(filter.Match("fileA.txt"));
    EXPECT_FALSE(filter.Match("file.txt"));
    EXPECT_FALSE(filter.Match("file12.txt"));
}

TEST(FilterTest, ComplexMask) {
    std::vector<std::string> masks = {"*test*"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("test_file.txt"));
    EXPECT_TRUE(filter.Match("unit_test.cpp"));
    EXPECT_TRUE(filter.Match("mytestfile.jpg"));
    EXPECT_FALSE(filter.Match("temp_file.txt"));
}

TEST(FilterTest, ToLowerUtility) {
    std::vector<std::string> masks = {"TEST.TXT"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("test.txt"));
    EXPECT_TRUE(filter.Match("TEST.TXT"));
    EXPECT_TRUE(filter.Match("Test.Txt"));
    EXPECT_FALSE(filter.Match("test.doc"));
}

TEST(FilterTest, StarMatchesAnything) {
    std::vector<std::string> masks = {"*"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("anyfile.txt"));
    EXPECT_TRUE(filter.Match(""));
    EXPECT_TRUE(filter.Match("file.with.dots.txt"));
}

TEST(FilterTest, ExtensionFilter) {
    std::vector<std::string> masks = {"*.cpp", "*.h", "*.hpp"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("main.cpp"));
    EXPECT_TRUE(filter.Match("header.h"));
    EXPECT_TRUE(filter.Match("header.hpp"));
    EXPECT_FALSE(filter.Match("source.c"));
    EXPECT_FALSE(filter.Match("document.txt"));
}

TEST(FilterTest, FixedFilename) {
    std::vector<std::string> masks = {"Makefile", "README.md"};
    Filter filter(masks);
    
    EXPECT_TRUE(filter.Match("Makefile"));
    EXPECT_TRUE(filter.Match("README.md"));
    EXPECT_TRUE(filter.Match("readme.md"));
    EXPECT_FALSE(filter.Match("Makefile.txt"));
    EXPECT_FALSE(filter.Match("README.txt"));
}