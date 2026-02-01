#include "block_cache.h"
#include "file_reader.h"
#include "hasher.h"

void BlockCache::SetHasher(Hasher* hasher) {
    hasher_ = hasher;
}

const std::string& BlockCache::Get(const std::string& file, std::size_t index, std::size_t block_size) {
    
    auto& hashes = cache_[file];

    if(index < hashes.size()) {
        return hashes[index];
    }

    auto data = ReadBlock(file, index, block_size);
    if(data.empty()) {
        static const std::string eof{};
        return eof;
    }

    hashes.push_back((*hasher_)(data));
    return hashes.back();
}