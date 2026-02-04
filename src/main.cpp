#include <boost/program_options.hpp>
#include <iostream>
#include "config.h"
#include "scanner.h"
#include "comparer.h"
#include "hasher.h"
#include "filter.h"



int main(int argc, char* argv[]) {

    namespace po = boost::program_options;

    std::vector<std::string> dirs, exclude, masks;
    std::size_t depth, block, min_size;
    std::string hash;

    po::options_description desc("Options");

    desc.add_options()
        ("dirs", po::value(&dirs)->multitoken(), "Folders to scan")
        ("exclude", po::value(&exclude)->multitoken(), "Exclude folders from scanning")
        ("depth", po::value(&depth)->default_value(0), "Scanning level")
        ("blocksize", po::value(&block)->default_value(DEFAULT_BLOCK_SIZE), "Block size")
        ("filesize", po::value(&min_size)->default_value(DEFAULT_MIN_FILE_SIZE), "Minimum size of file")
        ("masks", po::value(&masks)->multitoken(), "Masks of filenames")
        ("hash", po::value(&hash)->default_value("crc32"), "Hashing algorithm");

        po::variables_map vm;

        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        HashType ht = (hash == "md5") ? HashType::MD5 : HashType::CRC32;
        Hasher hasher(ht);

        Filter filter(min_size, masks);

        Scanner scanner;

        auto files = scanner.Scan(dirs, exclude, depth, filter);
        
        Comparer cmp(block, hasher);

        std::vector<bool> used(files.size(), false);

        for(size_t i=0; i < files.size(); ++i) {
            if(used[i]) {
                continue;
            }
            std::vector<size_t> group{i};

            for(size_t j = i +1; j < files.size(); ++j) {
                if (!used[j] && cmp.Equal(files[i], files[j])) {
                    used[j] = true;
                    group.push_back(j);
                }
            }
            if(group.size() > 1) {
                for(auto idx : group) {
                    std::cout << files[idx] << "\n";
                    std::cout << "\n";
                }
            }
        }

    return 0;
}