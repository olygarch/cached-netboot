#include "communication.h"
#include "common.h"
#include <string.h>

static uint32_t read_uint32_t(tcp::socket& socket, boost::asio::yield_context yield) {
    uint32_t netval;
    boost::asio::async_read(socket, boost::asio::buffer(&netval, 4), yield);
    return ntohl(netval);
}

static std::string read_string(tcp::socket& socket, boost::asio::yield_context yield) {
    size_t size = read_uint32_t(socket, yield);
    std::string str;
    str.resize(size);
    boost::asio::async_read(socket, boost::asio::buffer(&str[0], size), yield);
    return str;
}

ChunkDataPacket::ChunkDataPacket(const uint8_t* ptr, size_t size) {
    data.resize(size);
    memcpy(&data[0], ptr, size);
}

ChunkDataPacket::ChunkDataPacket(tcp::socket& socket, boost::asio::yield_context yield) {
    uint32_t size = read_uint32_t(socket, yield);
    data.resize(size);
    boost::asio::async_read(socket, boost::asio::buffer(&data[0], size), yield);
}

/*
hash_t ChunkDataPacket::get_hash() const {
    Hasher hasher(chunk_max_size);
    hasher.update(data.begin(), data.end());
    return hasher.get_strong_hash();
}
*/

void ChunkDataPacket::add_buffers(std::vector<boost::asio::const_buffer>& buffers) {
    netlength = htonl(data.size());
    buffers.emplace_back(&netlength, 4);
    buffers.emplace_back(&data[0], data.size());
}

Chunk ChunkDataPacket::get_chunk() const {
    return {data.size(), &data[0]};
}

ChunkListPacket::ChunkListPacket(tcp::socket& socket, boost::asio::yield_context yield) {
    size_t size = read_uint32_t(socket, yield);
    for (size_t i=0; i<size; i++) {
        chunks.emplace_back(socket, yield);
    }
}

void ChunkListPacket::add_buffers(std::vector<boost::asio::const_buffer>& buffers) {
    netlength = htonl(chunks.size());
    buffers.emplace_back(&netlength, 4);
    for (auto& chunk: chunks) {
        chunk.add_buffers(buffers);
    }
}

void NewChunkPacket::add_buffers(std::vector<boost::asio::const_buffer>& buffers) {
    chunk.add_buffers(buffers);
}

SendChunkPacket::SendChunkPacket(tcp::socket& socket, boost::asio::yield_context yield) {
    receiver_str = read_string(socket, yield);
    receiver = address::from_string(receiver_str);
    chunk = hash_t(socket, yield);
}

void SendChunkPacket::add_buffers(std::vector<boost::asio::const_buffer>& buffers) {
    receiver_str = receiver.to_string();
    netlength = htonl(receiver_str.size());
    buffers.emplace_back(&netlength, 4);
    buffers.emplace_back(&receiver_str[0], receiver_str.size());
    chunk.add_buffers(buffers);
}

GetFilePacket::GetFilePacket(tcp::socket& socket, boost::asio::yield_context yield) {
    name = read_string(socket, yield);
}

void GetFilePacket::add_buffers(std::vector<boost::asio::const_buffer>& buffers) {
    netlength = htonl(name.size());
    buffers.emplace_back(&netlength, 4);
    buffers.emplace_back(&name[0], name.size());
}

FileInfoPacket::FileInfoPacket(tcp::socket& socket, boost::asio::yield_context yield) {
    name = read_string(socket, yield);
    boost::asio::async_read(socket, boost::asio::buffer(&netfsize, 8), yield);
    size = be64toh(netfsize);
    chunk_list = ChunkListPacket(socket, yield);
}

void FileInfoPacket::add_buffers(std::vector<boost::asio::const_buffer>& buffers) {
    netlength = htonl(name.size());
    netfsize = htobe64(size);
    buffers.emplace_back(&netlength, 4);
    buffers.emplace_back(&name[0], name.size());
    buffers.emplace_back(&netfsize, 8);
    chunk_list.add_buffers(buffers);
}

ErrorPacket::ErrorPacket(tcp::socket& socket, boost::asio::yield_context yield) {
    boost::asio::async_read(socket, boost::asio::buffer(&code, 1), yield);
}

void ErrorPacket::add_buffers(std::vector<boost::asio::const_buffer>& buffers) {
    buffers.emplace_back(&code, 1);
}

const std::string ErrorPacket::get_as_string() const {
    switch (code) {
        case no_such_file:
            return "The file you requested does not exist!";
        case unknown_packet:
            return "Unknown packet type received!";
        default:
            return "Unknown error code!";
    };
}

packet_type get_packet_type(tcp::socket& socket, boost::asio::yield_context yield) {
    packet_type type;
    boost::asio::async_read(socket, boost::asio::buffer(&type, 1), yield);
    return type;
}
