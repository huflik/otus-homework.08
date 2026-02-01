#pragma once

#include "block_cache.h"
#include "hasher.h"

class Comparer {
    public:
        Comparer(std::size_t block_size, Hasher& hasher);
        bool Equal(const std::string& lh, const std::string& rh);

    private:
        std::size_t block_size_;
        BlockCache cache_;
};