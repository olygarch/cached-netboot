#include "common.h"
#include "hash.h"
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

hash_t::hash_t(tcp::socket& socket, boost::asio::yield_context yield) {
    boost::asio::async_read(socket, boost::asio::buffer(&weak_hash, 4), yield);
    boost::asio::async_read(socket, boost::asio::buffer(strong_hash), yield);
}

void hash_t::write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const {
    boost::asio::async_write(socket, boost::asio::buffer(&weak_hash, 4), yield);
    boost::asio::async_write(socket, boost::asio::buffer(strong_hash), yield);
}

void hash_t::print() const {
    fprintf(stderr, "%08x", weak_hash);
    for (int i=0; i<28; i++) fprintf(stderr, "%02x", (unsigned)strong_hash[i]);
    fprintf(stderr, "\n");
}

hash_t Chunk::get_hash() const {
    Hasher hasher(chunk_max_size);
    hasher.update(data, data+size);
    return hasher.get_strong_hash();
}
