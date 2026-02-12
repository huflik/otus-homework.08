#pragma once

#include "config.h"
#include "filter.h"
#include <boost/filesystem.hpp>
#include <map>
#include <vector>
#include <unordered_set>
#include "path_hash.h" 
class Scanner
{
public:
    explicit Scanner(const Config& config);   
    std::map<uintmax_t, std::vector<boost::filesystem::path>> Scan();

private:
    const Config& config_;
    Filter filter_;
    std::unordered_set<boost::filesystem::path, PathHash> seen_paths_;
    
    void ScanDirectory(const boost::filesystem::path& root, size_t current_depth, std::map<uintmax_t, std::vector<boost::filesystem::path>>& result);   
    bool IsExcluded(const boost::filesystem::path& path) const;   
    bool ScanSubdirectory(size_t current_depth) const;
};