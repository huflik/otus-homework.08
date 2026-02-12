#pragma once

#include <boost/filesystem.hpp>  
#include <unordered_map>         
#include <vector>                
#include <string>                
#include <fstream>               
#include <memory>  
#include "path_hash.h"              

class Hasher;

struct BlockKey
{
    boost::filesystem::path path;   
    size_t index;       

    bool operator==(const BlockKey& other) const
    {
        return path == other.path && index == other.index;
    }
};

struct BlockKeyHash
{
    std::size_t operator()(const BlockKey& k) const
    {
        return std::hash<std::string>()(k.path.string()) ^ (std::hash<size_t>()(k.index) << 1);
    }
};

struct FileHandle
{
    std::unique_ptr<std::ifstream> stream;           
    
    FileHandle(const boost::filesystem::path& p)
    {
        stream = std::make_unique<std::ifstream>(p.string(), std::ios::binary);
    }
};

class BlockCache
{
public:
    BlockCache(size_t block_size, std::unique_ptr<Hasher> hasher);
    ~BlockCache();
    std::string GetBlockHash(const boost::filesystem::path& file, size_t block_index);
    size_t GetBlockCount(const boost::filesystem::path& file);

private:
    size_t block_size_;  
    std::unique_ptr<Hasher> hasher_;  
    std::unordered_map<BlockKey, std::string, BlockKeyHash> hash_cache_; 
    std::unordered_map<boost::filesystem::path, size_t, PathHash> file_block_count_;
    std::unordered_map<boost::filesystem::path, std::shared_ptr<FileHandle>, PathHash> open_files_;
    
    std::string ReadAndHashBlock(const boost::filesystem::path& file, size_t index);
    std::shared_ptr<FileHandle> GetFileHandle(const boost::filesystem::path& file);
};