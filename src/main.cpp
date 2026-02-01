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


    return 0;
}