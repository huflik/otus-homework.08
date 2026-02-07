#pragma once

#include <string>     
#include <vector>     

class Filter
{
public:
    explicit Filter(const std::vector<std::string>& masks);
    bool Match(const std::string& filename) const;

private:
    std::vector<std::string> masks_; 
    
    static bool MatchOne(const std::string& name, const std::string& mask);   
    static std::string ToLower(const std::string& s);
};