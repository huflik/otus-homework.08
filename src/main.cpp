#include <iostream>           
#include <exception>          
#include "parser.h"           
#include "scanner.h"          
#include "hasher.h"      
#include "block_cache.h"      
#include "duplicate_finder.h" 
#include "utilities.h"

int main(int argc, char* argv[])
{
    try {

        Parser parser;
        Config config = parser.Parse(argc, argv);
        
		auto hasher = std::make_unique<Hasher>(config.hash_type);
        auto cache = std::make_unique<BlockCache>(config.block_size, std::move(hasher));
        auto duplicate_finder = std::make_unique<DuplicateFinder>(std::move(cache));
        
        Scanner scanner(config);
        auto files = scanner.Scan();
        
        auto duplicates = duplicate_finder->Find(files);
        
        PrintResults(duplicates);
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}