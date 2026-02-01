#pragma once

#include <vector>
#include <string>
#include "filter.h"

class Scanner {
    public:
        std::vector<std::string> Scan(const std::vector<std::string>& roots, const std::vector<std::string> exclude, std::size_t depth, const Filter& filter);
        
};