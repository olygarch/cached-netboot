#ifndef CN_COMMON_H
#define CN_COMMON_H
#include <stdint.h>
#include <unordered_set>
#include <unordered_map>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>

using namespace boost::asio::ip;

const static size_t chunk_max_size = 0x00100000;
const static short server_port = 5124;
const static short client_port = 8546; 

class sha224_t: public std::array<uint8_t, 28> {};

struct hash_t {
    uint32_t weak_hash;
    sha224_t strong_hash;
    hash_t() {}
    hash_t(tcp::socket& socket, boost::asio::yield_context yield);
    hash_t(uint32_t weak, sha224_t strong): weak_hash(weak), strong_hash(strong) {}
    void write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const;
    bool operator==(const hash_t& other) const {
        return weak_hash == other.weak_hash && strong_hash == other.strong_hash;
    }
    void print() const;
    uint32_t get_weak_hash() const;
};

namespace std {
    template <>
    class hash<hash_t>{
    public:
        size_t operator()(const hash_t& hash) const noexcept {
            return hash.weak_hash;
        }
    };
    template <>
    class hash<address>{
    public:
        size_t operator()(const address& addr) const noexcept {
            return std::hash<std::string>()(addr.to_string());
        }
    };
};

class ClientStatus {
public:
    boost::asio::strand strand;
    tcp::socket socket;
    std::unordered_set<hash_t> chunks_owned;
    std::unordered_set<hash_t> chunks_needed;
    ClientStatus() = delete;
    ClientStatus(tcp::socket socket, boost::asio::io_service& io_service): strand(io_service), socket(std::move(socket)) {}
};

class Chunk {
public:
    size_t size;
    const uint8_t* data;
    Chunk(size_t size, const uint8_t* data): size(size), data(data) {}
    Chunk(const uint8_t* begin, const uint8_t* end): size(end-begin), data(begin) {}
    hash_t get_hash() const;
};

/*uint32_t ntohl(uint32_t n) {
    uint8_t *np = (uint8_t *)&n;
    return ((uint32_t)np[0] << 24) |
           ((uint32_t)np[1] << 16) |
           ((uint32_t)np[2] <<  8) |
           ((uint32_t)np[3] <<  0);
}

uint32_t htonl(uint32_t h) {
    uint32_t n;
    uint8_t *np = (uint8_t *)&n;
    np[0] = (n >> 24) & 0xFF;
    np[1] = (n >> 16) & 0xFF;
    np[2] = (n >>  8) & 0xFF;
    np[3] = (n >>  0) & 0xFF;
    return n;
}
*/
#endif
