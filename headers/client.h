#ifndef CN_CLIENT_H
#define CN_CLIENT_H
#include <string>
#include "file.h"
#include "ui.h"
#include "communication.h"
#include "common.h"
#include <utility>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

using namespace boost::asio::ip;

template<class UI = DefaultUI>
class Client {
    std::string base_folder;
    std::unordered_map<std::string, File> files;
    address server_ip;
    std::unordered_map<hash_t, std::vector<File*>> chunk_files;
    std::unordered_set<hash_t> present_chunks;
    std::vector<std::string> files_to_get;
    UI ui;
    void run(bool forever);
public:
    Client(const address& server_ip, const std::string& base_folder, const std::vector<std::string>& files_to_get):
        base_folder(base_folder), server_ip(server_ip), files_to_get(files_to_get), ui({"Download status"}) {}
    void run_forever() {run(true);}
    void run_until_complete() {run(false);}
};

template<class UI>
void Client<UI>::run(bool forever) {
    using namespace std::placeholders;
    boost::asio::io_service io_service;
    tcp::socket server_socket(io_service);
    boost::asio::io_service::strand server_strand(io_service);

    auto chunk_data_sender = [this, &io_service] (const SendChunkPacket& packet, boost::asio::yield_context yield) {
        try {
            ChunkDataPacket output(chunk_files[packet.chunk][0]->get_chunk_data(packet.chunk));
            tcp::socket peer_socket(io_service);
            peer_socket.async_connect(tcp::endpoint(packet.receiver, client_port), yield);
            send_packet(peer_socket, yield, output);
        } catch (const std::exception& e) {
            ui.log("Send packet: " + std::string(e.what()));
        }
    };

    auto server_communication_handler = [this, &server_socket, &io_service, &chunk_data_sender] (boost::asio::yield_context yield) {
        try {
            server_socket.async_connect(tcp::endpoint(server_ip, server_port), yield);
            for (auto& x: files_to_get) {
                send_packet(server_socket, yield, GetFilePacket(x));
            }
            for (;;) {
                packet_type type = get_packet_type(server_socket, yield);
                switch (type) {
                    case file_info: {
                        FileInfoPacket packet(server_socket, yield);
                        files.emplace(
                            std::piecewise_construct,
                            std::forward_as_tuple(packet.name),
                            std::forward_as_tuple(base_folder + "/" + packet.name, packet.size));
                        files.at(packet.name).set_chunks_from_list(packet.chunk_list.chunks);
                        std::unordered_set<hash_t> needed_chunks(packet.chunk_list.chunks.begin(), packet.chunk_list.chunks.end());
                        for (auto& x: needed_chunks) {
                            chunk_files[x].push_back(&files.at(packet.name));
                        }
                        for (auto& x: files.at(packet.name).get_present_chunks()) {
                            present_chunks.insert(x);
                        }
                        send_packet(server_socket, yield, ChunkListPacket(present_chunks.begin(), present_chunks.end()));
                        break;
                    }
                    case send_chunk: {
                        boost::asio::spawn(io_service, std::bind(chunk_data_sender, SendChunkPacket(server_socket, yield), _1));
                        break;
                    }
                    case error: {
                        ui.log("Received error from server: " + ErrorPacket(server_socket, yield).get_as_string());
                        break;
                    }
                    default: {
                        ui.log("Unknown packet type from server: " + std::to_string(type));
                        ErrorPacket answer(unknown_packet);
                        send_packet(server_socket, yield, answer);
                    }
                }
                ui.report_status(files);
            }
        } catch (const std::exception& e) {
            ui.log("Server communication: " + std::string(e.what()));
        }
    };

    auto new_chunk_sender = [this, &server_socket] (const hash_t& hash, boost::asio::yield_context yield) {
        try {
            send_packet(server_socket, yield, NewChunkPacket(hash));
        } catch (const std::exception& e) {
            ui.log("Error sending update to server: " + std::string(e.what()));
        }
    };

    auto peer_connect_handler = [this, forever, &server_strand, &io_service, &new_chunk_sender] (tcp::socket& socket, boost::asio::yield_context yield) {
        try {
            for (;;) {
                packet_type type = get_packet_type(socket, yield);
                switch (type) {
                    case chunk_data: {
                        ChunkDataPacket packet(socket, yield);
                        const hash_t& hash = packet.get_chunk().get_hash();
                        if (!chunk_files.count(hash)) {
                            ui.log("Unknown chunk received!");
                            break;
                        }
                        for (auto x: chunk_files.at(hash)) {
                            x->write_chunk(packet.get_chunk(), hash);
                        }
                        boost::asio::spawn(server_strand, std::bind(new_chunk_sender, hash, _1));
                        break;
                    }
                    case error: {
                        ui.log("Received error from client: " + ErrorPacket(socket, yield).get_as_string());
                        break;
                    }
                    default: {
                        ui.log("Unknown packet type from client: " + std::to_string(type));
                    }
                }
                ui.report_status(files);
                if (forever) continue;
                if (chunk_files.size() == present_chunks.size()) {
                    ui.log("Stopping...");
                    io_service.stop();
                    break;
                }
            }
        } catch (const std::exception& e) {
            ui.log("Client communication: " + std::string(e.what()));
        }
    };

    auto peer_connect_listener = [this, &io_service, &peer_connect_handler] (boost::asio::yield_context yield) {
        try {
            tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), client_port));
            for (;;) {
                tcp::socket socket(io_service);
                acceptor.async_accept(socket, yield);
                boost::asio::spawn(io_service, std::bind(peer_connect_handler, std::move(socket), _1));
            }
        } catch (const std::exception& e) {
            ui.log("Client accept: " + std::string(e.what()));
        }
    };

    boost::asio::spawn(io_service, peer_connect_listener);
    boost::asio::spawn(server_strand, server_communication_handler);
    io_service.run();
}

#endif
