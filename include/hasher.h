#pragma once

#include <cstdint>     
#include <string>      
#include <vector>      
#include "config.h"     

class Hasher
{
public:
    explicit Hasher(HashType hash_type);   
    std::string HashBlock(const char* data, size_t size);

private:
    HashType hash_type_; 

    std::string HashBlockCrc32(const char* data, size_t size);
    std::string HashBlockMd5(const char* data, size_t size);
    static std::string BytesToHex(const uint8_t* data, size_t size);
};
