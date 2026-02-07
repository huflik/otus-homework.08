#pragma once

#include <memory>
#include <map>                
#include <string>             
#include <vector>             
#include "comparator.h"

class DuplicateFinder {
public:
    explicit DuplicateFinder(std::unique_ptr<BlockCache> cache);  
    std::vector<std::vector<std::string>> Find(const std::map<uintmax_t, std::vector<std::string>>& groups);
    
private:
    std::unique_ptr<BlockCache> cache_;        
    std::unique_ptr<Comparator> comparator_;
    
    DuplicateFinder(const DuplicateFinder&) = delete;
    DuplicateFinder& operator=(const DuplicateFinder&) = delete;   
    DuplicateFinder(DuplicateFinder&&) = default;
    DuplicateFinder& operator=(DuplicateFinder&&) = default;
};
