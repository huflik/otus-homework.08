#pragma once

#include <boost/filesystem.hpp>  
#include <vector>                
#include <string>               
#include "block_cache.h"

class Comparator
{
public:
    explicit Comparator(BlockCache& cache);
    bool Equals(const boost::filesystem::path& a, const boost::filesystem::path& b);   
    std::vector<std::vector<boost::filesystem::path>> FindDuplicates(const std::vector<boost::filesystem::path>& files);

private:
    BlockCache& cache_;
    
};