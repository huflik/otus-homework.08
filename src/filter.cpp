#include <algorithm>  
#include <cctype>     
#include "filter.h"

Filter::Filter(const std::vector<std::string>& masks)
{
    masks_.reserve(masks.size());
    
    for (const auto& m : masks) {
        masks_.push_back(ToLower(m));
    }
}

bool Filter::Match(const std::string& filename) const
{
    if (masks_.empty()) {
        return true;
    }

    std::string name = ToLower(filename);

    for (const auto& mask : masks_) {
        if (MatchOne(name, mask)) {
            return true;
        }
    }

    return false;
}

std::string Filter::ToLower(const std::string& s)
{
    std::string r = s; 
    
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return r;
}
 
bool Filter::MatchOne(const std::string& name, const std::string& mask)
{
    size_t n = name.size();  
    size_t m = mask.size();  
    
    size_t i = 0;            
    size_t j = 0;            
    
    size_t star_pos = std::string::npos;  
    size_t name_pos = 0;                

    while (i < n) {
        if (j < m && mask[j] == '*') {
            star_pos = j++;         
            name_pos = i;            
        }
        else if (j < m && (mask[j] == '?' || mask[j] == name[i])) {
            ++i;  
            ++j;  
        }
        else if (star_pos != std::string::npos) {
            j = star_pos + 1;  
            i = ++name_pos;    
        }
        else {
            return false;  
        }
    }

    while (j < m && mask[j] == '*') {
        ++j;
    }

    return j == m;
}