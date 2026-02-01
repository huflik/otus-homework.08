#include <boost/filesystem.hpp>
#include "scanner.h"

std::vector<std::string> Scanner::Scan(const std::vector<std::string>& roots, const std::vector<std::string> exclude, std::size_t depth, const Filter& filter) {
    
    std::vector<std::string> result;

    for(const auto& root : roots) {
        boost::filesystem::recursive_directory_iterator it(root), end;
        
        while (it != end) {
            if(depth && it.depth() > depth) {
                it.pop();
                continue;
            }

            if(boost::filesystem::is_regular_file(*it)) {
                const auto size = boost::filesystem::file_size(*it);
                const auto path = it->path().string();

                if(filter.Accept(path, size)) {
                    result.push_back(path);               
                }
            }

            ++it;
        }        
    }

    return result;
}