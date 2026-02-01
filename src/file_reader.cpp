#include <fstream>
#include "file_reader.h"

std::vector<char> ReadBlock(const std::string& file, std::size_t index, std::size_t size) {
    
    std::ifstream in(file, std::ios::binary);
    if(!in) {
        return {};
    }

    in.seekg(index * size);

    std::vector<char> buff(size);
    in.read(buff.data(), size);

    buff.resize(in.gcount());
    return buff;

}