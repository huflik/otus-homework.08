#pragma once

#include "config.h"
#include "filter.h"
#include <boost/filesystem.hpp>
#include <map>
#include <vector>
#include <unordered_set>

class Scanner
{
public:
    explicit Scanner(const Config& config);   
    std::map<uintmax_t, std::vector<std::string>> Scan();

private:
    const Config& config_;
    Filter filter_;
    std::unordered_set<std::string> seen_paths_;
    
    void ScanDirectory(const boost::filesystem::path& root, size_t current_depth, std::map<uintmax_t, std::vector<std::string>>& result);   
    bool IsExcluded(const boost::filesystem::path& path) const;   
    bool ScanSubdirectory(size_t current_depth) const;
};