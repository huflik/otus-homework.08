#include <iostream>
#include "scanner.h"

Scanner::Scanner(const Config& config) : config_(config), filter_(config.masks)
{
}

std::map<uintmax_t, std::vector<boost::filesystem::path>> Scanner::Scan()
{
    std::map<uintmax_t, std::vector<boost::filesystem::path>> result;
    seen_paths_.clear();
    
    for (const auto& dir : config_.include_dirs) {
        if (!boost::filesystem::exists(dir) || 
            !boost::filesystem::is_directory(dir)) {
            std::cerr << "Warning: Not a directory or doesn't exist: " << dir << "\n";
            continue;
        }
        
        try {
            ScanDirectory(dir, 0, result);
        }
        catch (const std::exception& e) {
            std::cerr << "Error scanning directory " << dir << ": " << e.what() << "\n";
        }
    }
    
    return result;
}

bool Scanner::ScanSubdirectory(size_t current_depth) const
{
    return (current_depth + 1) <= config_.depth;
}

void Scanner::ScanDirectory(const boost::filesystem::path& root, size_t current_depth, std::map<uintmax_t, std::vector<boost::filesystem::path>>& result)
{
    if (!boost::filesystem::exists(root) || 
        !boost::filesystem::is_directory(root)) {
        return;
    }
    
    if (IsExcluded(root)) {
        return;
    }
    
    try {
        boost::filesystem::directory_iterator end;
        
        for (boost::filesystem::directory_iterator it(root); it != end; ++it) {
            const auto& path = it->path();
            
            try {
                if (boost::filesystem::is_symlink(path)) {
                    continue;
                }
                
                if (boost::filesystem::is_directory(path)) {
                    if (!ScanSubdirectory(current_depth)) {
                        continue;
                    }
                    
                    ScanDirectory(path, current_depth + 1, result);
                    continue;
                }
                
                if (!boost::filesystem::is_regular_file(path)) {
                    continue;
                }
                
                if (IsExcluded(path)) {
                    continue;
                }

                boost::system::error_code ec{};
                uintmax_t size = boost::filesystem::file_size(path, ec);
                if (ec || size <= config_.min_file_size) {
                    continue;
                }               
                
                if (!filter_.Match(path.filename().string())) {
                    continue;
                }
                
                std::string canonical_path;
                try {
                    canonical_path = boost::filesystem::canonical(path).string();
                }
                catch (...) {
                    canonical_path = boost::filesystem::absolute(path).string();
                }
                
                if (seen_paths_.count(canonical_path)) {
                    continue;
                }
                
                seen_paths_.insert(canonical_path);
                
                result[size].push_back(canonical_path);
                
            }
            catch (const std::exception& e) {
                std::cerr << "Skipping problematic files/directories. Error: " << e.what() << "\n";
                continue;
            }
        }
    }
    catch (const boost::filesystem::filesystem_error& e) {
        std::cerr << "Skipping directories with access errors. Error: " << e.what() << "\n";
        return;
    }
}

bool Scanner::IsExcluded(const boost::filesystem::path& path) const
{
        if (config_.exclude_dirs.empty()) {
        return false;
    }
    
    try {
        boost::filesystem::path abs_path = boost::filesystem::absolute(path).lexically_normal();
        
        for (const auto& ex_dir : config_.exclude_dirs) {
            try {
                boost::filesystem::path abs_ex_dir = boost::filesystem::absolute(ex_dir).lexically_normal();
                
                auto relative = abs_path.lexically_relative(abs_ex_dir);
                
                if (!relative.empty() && relative.native()[0] != '.') {
                    return true; 
                }
                
                if (relative == ".") {
                    return true; 
                }
            }
            catch (...) {
                continue; 
            }
        }
    }
    catch (...) {
        return false; 
    }
    
    return false;
}