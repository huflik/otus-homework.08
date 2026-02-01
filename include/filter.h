#pragma once

#include <vector>
#include <string>
#include <boost/regex.hpp>

class Filter {
    public:
        Filter(std::size_t min_size, const std::vector<std::string>& masks);
        bool Accept(const std::string& path, std::size_t size) const;

    private:
        std::size_t min_size_;
        std::vector<boost::regex> masks_;
};