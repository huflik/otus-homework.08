#include "duplicate_finder.h"

DuplicateFinder::DuplicateFinder(std::unique_ptr<BlockCache> cache) : cache_(std::move(cache))
{
    if (!cache_) {
        throw std::invalid_argument("BlockCache cannot be null");
    }
    
    comparator_ = std::make_unique<Comparator>(*cache_);
}

std::vector<std::vector<std::string>> DuplicateFinder::Find(const std::map<uintmax_t, std::vector<std::string>>& groups)
{
    std::vector<std::vector<std::string>> result;
    
    for (const auto& [size, files] : groups) {
        if (files.size() < 2) {
            continue;
        }

        auto duplicates = comparator_->FindDuplicates(files);
        
        result.insert(result.end(), 
                     std::make_move_iterator(duplicates.begin()),
                     std::make_move_iterator(duplicates.end()));
    }
    
    return result;
}
