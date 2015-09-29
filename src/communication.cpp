#include "communication.h"
#include "common.h"
#include <string.h>

static void write_uint32_t(tcp::socket& socket, boost::asio::yield_context yield, uint32_t val) {
    uint32_t netval = htonl(val);
    boost::asio::async_write(socket, boost::asio::buffer(&netval, 4), yield);
}

static uint32_t read_uint32_t(tcp::socket& socket, boost::asio::yield_context yield) {
    uint32_t netval;
    boost::asio::async_read(socket, boost::asio::buffer(&netval, 4), yield);
    return ntohl(netval);
}

static void write_string(tcp::socket& socket, boost::asio::yield_context yield, const std::string& str) {
    write_uint32_t(socket, yield, str.size()-1);
    boost::asio::async_write(socket, boost::asio::buffer(&str[0], str.size()-1), yield);
}

static std::string read_string(tcp::socket& socket, boost::asio::yield_context yield) {
    size_t size = read_uint32_t(socket, yield);
    std::string str;
    str.resize(size+1);
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

void ChunkDataPacket::write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const {
    write_uint32_t(socket, yield, data.size());
    boost::asio::async_write(socket, boost::asio::buffer(&data[0], data.size()), yield);
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

void ChunkListPacket::write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const {
    write_uint32_t(socket, yield, chunks.size());
    for (auto chunk: chunks) {
        chunk.write_to_socket(socket, yield);
    }
}

void NewChunkPacket::write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const {
    chunk.write_to_socket(socket, yield);
}

SendChunkPacket::SendChunkPacket(tcp::socket& socket, boost::asio::yield_context yield) {
    receiver.from_string(read_string(socket, yield));
    chunk = hash_t(socket, yield);
}

void SendChunkPacket::write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const {
    write_string(socket, yield, receiver.to_string());
    chunk.write_to_socket(socket, yield);
}


GetFilePacket::GetFilePacket(tcp::socket& socket, boost::asio::yield_context yield) {
    name = read_string(socket, yield);
}

void GetFilePacket::write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const {
    write_string(socket, yield, name);
}

FileInfoPacket::FileInfoPacket(tcp::socket& socket, boost::asio::yield_context yield) {
    name = read_string(socket, yield);
    size = read_uint32_t(socket, yield);
    chunk_list = ChunkListPacket(socket, yield);
}

void FileInfoPacket::write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const {
    write_string(socket, yield, name);
    write_uint32_t(socket, yield, size);
    chunk_list.write_to_socket(socket, yield);
}

ErrorPacket::ErrorPacket(tcp::socket& socket, boost::asio::yield_context yield) {
    boost::asio::async_read(socket, boost::asio::buffer(&code, 1), yield);
}

void ErrorPacket::write_to_socket(tcp::socket& socket, boost::asio::yield_context yield) const {
    boost::asio::async_write(socket, boost::asio::buffer(&code, 1), yield);
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
