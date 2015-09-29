#include "common.h"
#include "hash.h"
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

hash_t::hash_t(tcp::socket& socket, boost::asio::yield_context yield) {
    boost::asio::async_read(socket, boost::asio::buffer(weak_hash), yield);
    boost::asio::async_read(socket, boost::asio::buffer(strong_hash), yield);
}

void hash_t::write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const {
    boost::asio::async_write(socket, boost::asio::buffer(weak_hash), yield);
    boost::asio::async_write(socket, boost::asio::buffer(strong_hash), yield);
}

hash_t Chunk::get_hash() const {
    Hasher hasher(chunk_max_size);
    hasher.update(data, data+size);
    return hasher.get_strong_hash();
}
