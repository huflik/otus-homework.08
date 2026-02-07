#include <vector>        
#include <cstring>        
#include <stdexcept>      
#include "block_cache.h"
#include "hasher.h"

BlockCache::BlockCache(size_t block_size, std::unique_ptr<Hasher> hasher) : block_size_(block_size), hasher_(std::move(hasher))
{
    if (!hasher_) {
        throw std::invalid_argument("HashEngine cannot be null");
    }
    if (block_size_ == 0) {
        throw std::invalid_argument("Block size must be greater than 0");
    }
}

BlockCache::~BlockCache() {}

size_t BlockCache::GetBlockCount(const boost::filesystem::path& file)
{
    auto key = file.string(); 
    
    auto it = file_block_count_.find(key);
    if (it != file_block_count_.end())
        return it->second;

    try {
        uintmax_t size = boost::filesystem::file_size(file);
        
        size_t count = (size + block_size_ - 1) / block_size_;
        
        file_block_count_[key] = count;
        return count;
    }
    catch (const boost::filesystem::filesystem_error& e) {
        throw std::runtime_error("Cannot get file size: " + std::string(e.what()));
    }
}

std::shared_ptr<FileHandle> BlockCache::GetFileHandle(const boost::filesystem::path& file)
{
    auto key = file.string();
    
    auto it = open_files_.find(key);
    if (it != open_files_.end()) {
        return it->second; 
    }
    
    try {
        auto handle = std::make_shared<FileHandle>(file);
        
        if (!handle->stream || !*handle->stream) {
            throw std::runtime_error("Cannot open file: " + file.string());
        }
        
        open_files_[key] = handle;
        return handle;
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to open file " + file.string() + ": " + e.what());
    }
}

std::string BlockCache::ReadAndHashBlock(const boost::filesystem::path& file, size_t index)
{
    try {
        auto handle = GetFileHandle(file);
        auto& stream = *handle->stream;
        
        std::streampos pos = static_cast<std::streampos>(index * block_size_);
        
        stream.seekg(pos);
        
        if (!stream) {
            throw std::runtime_error("Cannot seek to position in file: " + file.string());
        }
        
        std::vector<char> buffer(block_size_, 0);     
        stream.read(buffer.data(), block_size_);
        
        return hasher_->HashBlock(buffer.data(), block_size_);
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error reading block from " + file.string() + ": " + e.what());
    }
}

std::string BlockCache::GetBlockHash(const boost::filesystem::path& file, size_t block_index)
{
    BlockKey key{file.string(), block_index};

    auto it = hash_cache_.find(key);
    if (it != hash_cache_.end()) {
        return it->second;  
    }

    std::string hash = ReadAndHashBlock(file, block_index);
    
    hash_cache_[key] = hash;
    return hash;
}