#ifndef CN_COMMUNICATION_H
#define CN_COMMUNICATION_H
#include <stdint.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <vector>
#include "common.h"

using namespace boost::asio::ip;

enum packet_type: uint8_t {
    chunk_data,
    chunk_list,
    new_chunk,
    send_chunk,
    get_file,
    file_info,
    error = 255
};

enum error_code: uint8_t {
    no_such_file,
    unknown_packet = 255
};

class ChunkDataPacket {
public:
    const static packet_type type = chunk_data;
    std::vector<uint8_t> data;
    ChunkDataPacket(const uint8_t* ptr, size_t size);
    ChunkDataPacket(const Chunk& chunk): ChunkDataPacket(chunk.data, chunk.size) {}
    ChunkDataPacket(tcp::socket& socket, boost::asio::yield_context yield);
//    hash_t get_hash() const;
    void write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const;
    Chunk get_chunk() const;
};

class ChunkListPacket {
public:
    const static packet_type type = chunk_list;
    std::vector<hash_t> chunks;
    ChunkListPacket() {}
    template<typename Iterator, typename boost::enable_if<boost::is_same<typename std::iterator_traits<Iterator>::value_type, hash_t>, int>::type = 0>
    ChunkListPacket(Iterator begin, Iterator end) {
        for (Iterator it=begin; it!=end; it++) {
            chunks.push_back(*it);
        }
    }
    ChunkListPacket(tcp::socket& socket, boost::asio::yield_context yield);
    void write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const;
};

class NewChunkPacket {
public:
    const static packet_type type = new_chunk;
    hash_t chunk;
    NewChunkPacket(hash_t chunk): chunk(chunk) {}
    NewChunkPacket(tcp::socket& socket, boost::asio::yield_context yield): chunk(socket, yield) {}
    void write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const;
};

class SendChunkPacket {
public:
    const static packet_type type = send_chunk;
    address receiver;
    hash_t chunk;
    SendChunkPacket(address receiver, hash_t chunk): receiver(receiver), chunk(chunk) {}
    SendChunkPacket(tcp::socket& socket, boost::asio::yield_context yield);
    void write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const;
};

class GetFilePacket {
public:
    const static packet_type type = get_file;
    std::string name;
    GetFilePacket(const std::string& name): name(name) {}
    GetFilePacket(tcp::socket& socket, boost::asio::yield_context yield);
    void write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const;
};

class FileInfoPacket {
public:
    const static packet_type type = file_info;
    std::string name;
    uint32_t size;
    ChunkListPacket chunk_list;
    template<typename Iterator, typename boost::enable_if<boost::is_same<typename std::iterator_traits<Iterator>::value_type, hash_t>, int>::type = 0>
    FileInfoPacket(std::string name, uint32_t size, Iterator begin, Iterator end): name(name), size(size), chunk_list(begin, end) {}
    FileInfoPacket(tcp::socket& socket, boost::asio::yield_context yield);
    void write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const;
};

class ErrorPacket {
public:
    const static packet_type type = error;
    error_code code;
    ErrorPacket(error_code code): code(code) {}
    ErrorPacket(tcp::socket& socket, boost::asio::yield_context yield);
    void write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const;
    const std::string get_as_string() const;
};

template<typename PacketType>
void send_packet(tcp::socket& socket, boost::asio::yield_context yield, const PacketType& packet) {
    packet_type type = PacketType::type;
    boost::asio::async_write(socket, boost::asio::buffer(&type, 1), yield);
    packet.write_to_socket(socket, yield);
}

packet_type get_packet_type(tcp::socket& socket, boost::asio::yield_context yield);
#endif
