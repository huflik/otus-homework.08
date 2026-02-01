#include "comparer.h"

Comparer::Comparer(std::size_t block_size, Hasher& hasher) : block_size_(block_size) {
    
    cache_.SetHasher(&hasher);
}

bool Comparer::Equal(const std::string& lh, const std::string& rh) {
    
    std::size_t index = 0;

    while(true) {
        const auto& hlh = cache_.Get(lh, index, block_size_);
        const auto& hrh = cache_.Get(rh, index, block_size_);

        if(hlh != hrh) {
            return false;
        }
        if(hlh.empty()) {
            return true;
        }

        ++index;
    }
}