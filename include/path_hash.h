#pragma once

#include <boost/filesystem.hpp>
#include <string>
#include <unordered_map>

struct PathHash
{
    std::size_t operator()(const boost::filesystem::path& p) const
    {
        return std::hash<std::string>()(p.string());
    }
};