#pragma once

#include <vector>
#include <string>

enum class HashType {
    CRC32,
    MD5
};

class Hasher {
    public:
        explicit Hasher(HashType type);
        std::string operator()(const std::vector<char>& data) const;

    private:
        HashType type_;
    
};
