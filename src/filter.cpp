#include <boost/filesystem.hpp>
#include "filter.h"

Filter::Filter(std::size_t min_size, const std::vector<std::string>& masks): min_size_(min_size) {

    for(const auto& m : masks) {
        masks_.emplace_back(m, boost::regex::icase);
    }
}

bool Filter::Accept(const std::string& path, std::size_t size) const {

    if(size < min_size_) {
        return false;
    }
    if(masks_.empty()) {
        return true;
    }

    const auto name = boost::filesystem::path(path).filename().string();

    for(const auto& p : masks_) {
        if(boost::regex_match(name, p)) {
            return true;
        }
    }

    return false;
}