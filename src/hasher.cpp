#include <boost/crc.hpp>               
#include <boost/uuid/detail/md5.hpp>   
#include <sstream>                    
#include <iomanip>                     
#include <fstream>                     
#include <vector>                    
#include <cstring>                     
#include "hasher.h"


Hasher::Hasher(HashType hash_type) : hash_type_(hash_type){}

std::string Hasher::HashBlock(const char* data, size_t size)
{
    if (hash_type_ == HashType::CRC32) {
        return HashBlockCrc32(data, size);
    } else {
        return HashBlockMd5(data, size);
    }
}

std::string Hasher::HashBlockCrc32(const char* data, size_t size)
{
    boost::crc_32_type crc;
    
    crc.process_bytes(data, size);
    
    uint32_t sum = crc.checksum();
    
    std::stringstream ss;
    ss << std::hex                     
       << std::setfill('0')           
       << std::setw(8)                
       << sum;                         
    
    return ss.str();
}

std::string Hasher::HashBlockMd5(const char* data, size_t size)
{
    boost::uuids::detail::md5 hash;
    
    hash.process_bytes(data, size);
    
    boost::uuids::detail::md5::digest_type digest;
    hash.get_digest(digest);
    
    return BytesToHex(reinterpret_cast<const uint8_t*>(&digest), sizeof(digest));
}

std::string Hasher::BytesToHex(const uint8_t* data, size_t size)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');  
    
    for (size_t i = 0; i < size; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    
    return ss.str();
}