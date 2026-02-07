#include <boost/program_options.hpp>  
#include <iostream>                   
#include <algorithm>                  
#include "parser.h"

namespace po = boost::program_options;  

HashType Parser::ParseHashType(const std::string& str)
{
    std::string lower = str;
    
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    if (lower == "crc32") {
        return HashType::CRC32;
    } else if (lower == "md5") {
        return HashType::MD5;
    }
    
    throw std::runtime_error("Unknown hash type: " + str + ". Supported: crc32, md5");
}

Config Parser::Parse(int argc, char* argv[])
{
    Config config;
    
    po::options_description desc("Utility for finding duplicate files\nAllowed options");
    
    desc.add_options()
        ("help,h", "show help message")


        ("include,i", po::value<std::vector<std::string>>()->multitoken(),
         "directories to scan (can be multiple)")

        ("exclude,e", po::value<std::vector<std::string>>()->multitoken(),
         "directories to exclude (can be multiple)")

        ("depth,d", po::value<size_t>(&config.depth)->default_value(0),
         "scan depth (0 = only specified directory)")

        ("min-size", po::value<uintmax_t>(&config.min_file_size)->default_value(2),
         "minimum file size in bytes")

        ("mask,m", po::value<std::vector<std::string>>()->multitoken(),
         "file masks (case-insensitive, can be multiple)")

        ("block,b", po::value<size_t>(&config.block_size)->default_value(4096),
         "block size for reading files")

        ("hash", po::value<std::string>()->default_value("crc32"),
         "hash algorithm: crc32 or md5")
    ;
    
    po::positional_options_description positional;
    positional.add("include", -1);
    
    po::variables_map vm;
    
    try {
        po::store(po::command_line_parser(argc, argv)
                  .options(desc)
                  .positional(positional)
                  .run(), 
                  vm);
        po::notify(vm);
        
        if (vm.count("help")) {
            std::cout << desc << "\n";
            std::exit(0);
        }
        
        if (vm.count("include")) {
            auto dirs = vm["include"].as<std::vector<std::string>>();
            config.include_dirs.reserve(dirs.size());
            for (const auto& d : dirs) {
                config.include_dirs.emplace_back(d);
            }
        } else {
            config.include_dirs.emplace_back(".");
        }
        
        if (vm.count("exclude")) {
            auto dirs = vm["exclude"].as<std::vector<std::string>>();
            config.exclude_dirs.reserve(dirs.size());
            for (const auto& d : dirs) {
                config.exclude_dirs.emplace_back(d);
            }
        }
        
        if (vm.count("mask")) {
            config.masks = vm["mask"].as<std::vector<std::string>>();
        }
        
        config.hash_type = ParseHashType(vm["hash"].as<std::string>());
        
        if (!config.Validate()) {
            throw std::runtime_error("Invalid configuration");
        }
        
        if (config.block_size == 0) {
            throw std::runtime_error("Block size must be greater than 0");
        }
        
    } catch (const po::error& e) {
        throw std::runtime_error("Command line parsing error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Configuration error: " + std::string(e.what()));
    }
    
    return config;
}
