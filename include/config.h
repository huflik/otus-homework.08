#pragma once  // Защита от многократного включения

#include <boost/filesystem.hpp>  
#include <string>                
#include <vector>                
#include <cstddef>               

enum class HashType
{
    CRC32,  
    MD5     
};

struct Config
{
    std::vector<boost::filesystem::path> include_dirs;
    std::vector<boost::filesystem::path> exclude_dirs;
    size_t depth = 0;
    uintmax_t min_file_size = 1;
    std::vector<std::string> masks;
    size_t block_size = 4096;
    HashType hash_type = HashType::CRC32;
    
    bool Validate() const
    {
        if (include_dirs.empty()) {
            return false;
        }
        
        if (block_size == 0) {
            return false;
        }
        
        return true;
    }
};
