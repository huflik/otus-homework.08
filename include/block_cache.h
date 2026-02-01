#pragma once

#include <unordered_map>
#include <vector>
#include <string>

class BlockCache {
    public:
        const std::string& Get(const std::string& file, std::size_t index, std::size_t block_size);
        void SetHasher(class Hasher* hasher);

    private:
        std::unordered_map<std::string, std::vector<std::string>> cache_;
        Hasher* hasher_{nullptr};
};