#include <iostream>
#include "comparator.h"
#include "block_cache.h" 

Comparator::Comparator(BlockCache& cache) : cache_(cache){}

bool Comparator::Equals(const boost::filesystem::path& a, const boost::filesystem::path& b)
{
    try {
        size_t blocks_a = cache_.GetBlockCount(a);
        size_t blocks_b = cache_.GetBlockCount(b);
        
        if (blocks_a != blocks_b) {
            return false; 
        }
        
        for (size_t i = 0; i < blocks_a; ++i) {
            std::string ha = cache_.GetBlockHash(a, i);
            std::string hb = cache_.GetBlockHash(b, i);
            
            if (ha != hb) {
                return false;
            }           
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading files. Files " << a.string() << "and " << b.string() << " are considered different" << e.what() << "\n";
        return false;
    }
}

std::vector<std::vector<std::string>> Comparator::FindDuplicates(const std::vector<std::string>& files)
{
    std::vector<std::vector<std::string>> result; 
    
    if (files.size() < 2) {
        return result; 
    }
    
    std::vector<std::string> remaining_files = files;
    
    std::vector<bool> processed(remaining_files.size(), false);
    
    for (size_t i = 0; i < remaining_files.size(); ++i) {
        if (processed[i]) {
            continue;
        }
        
        std::vector<std::string> duplicate_group;
        duplicate_group.push_back(remaining_files[i]);
        processed[i] = true;
        
        for (size_t j = i + 1; j < remaining_files.size(); ++j) {
            if (processed[j]) {
                continue;
            }
            
            if (Equals(remaining_files[i], remaining_files[j])) {
                duplicate_group.push_back(remaining_files[j]);
                processed[j] = true;
            }
        }
        
        if (duplicate_group.size() > 1) {
            result.push_back(std::move(duplicate_group)); 
        }
    }
    
    return result;
}