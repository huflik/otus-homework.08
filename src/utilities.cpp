#include "utilities.h"

void PrintResults(const std::vector<std::vector<boost::filesystem::path>>& duplicates)
{
    if (duplicates.empty()) {
        std::cout << "No duplicate files found.\n";
        return;
    }
    
    bool first_group = true;
    for (const auto& group : duplicates) {
        if (!first_group) {
            std::cout << '\n';
        }
        first_group = false;
        
        for (const auto& file : group) {
            std::cout << file << '\n';
        }
    }
}