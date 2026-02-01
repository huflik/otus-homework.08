#include <boost/crc.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <sstream>
#include <iomanip>
#include "hasher.h"

Hasher::Hasher(HashType type) : type_(type) {}

std::string Hasher::operator()(const std::vector<char>& data) const {
    if(type_ == HashType::CRC32) {
        boost::crc_32_type crc;
        crc.process_bytes(data.data(), data.size());
        return std::to_string(crc.checksum());
    }

    boost::uuids::detail::md5 md5;
    md5.process_bytes(data.data(), data.size());

    boost::uuids::detail::md5::digest_type digest;
    md5.get_digest(digest);

    std::ostringstream out;
    for(auto p : digest) {
        out << std::hex << std::setw(8) << std::setfill('0') << p;
    }

    return out.str();
}