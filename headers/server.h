#ifndef CN_SERVER_H
#define CN_SERVER_H
#include "common.h"
#include "communication.h"
#include "ui.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <queue>

using namespace boost::asio::ip;


template<class UI = DefaultUI>
class Server {
    std::string base_dir;
    std::unordered_multiset<address> is_busy;
    std::unordered_map<address, ClientStatus> clients;
    std::unordered_map<std::string, FileInfoPacket> files;
    UI ui;
    std::unordered_map<address, std::pair<address, hash_t>> get_chunks_to_send() const;
public:
    Server(std::string base_dir): base_dir(base_dir), ui({"Client status"}) {}
    void run();
};

using namespace boost::filesystem;

template<class UI>
std::unordered_map<address, std::pair<address, hash_t>> Server<UI>::get_chunks_to_send() const {
    volatile size_t client_no = clients.size();
    std::unordered_map<address, size_t> addr_to_id;
    std::vector<address> id_to_addr;
    for (auto& c: clients) {
        addr_to_id.emplace(c.first, addr_to_id.size());
        id_to_addr.push_back(c.first);
    }
    std::vector<std::vector<int>> fw_graph;
    fw_graph.resize(client_no);
    for (auto& c: clients) {
        for (auto& oth: clients) {
            for (auto& chunk: c.second.chunks_owned) {
                if (oth.second.chunks_needed.count(chunk) && !oth.second.chunks_owned.count(chunk)) {
                    fw_graph[addr_to_id[c.first]].push_back(addr_to_id[oth.first]);
                    break;
                }
            }
        }
    }

    std::vector<int> fw_match(client_no, -1);
    std::vector<int> bw_match(client_no, -1);
    for (;;) {
        std::vector<int> parent(client_no, -1);
        bool improved = false;
        for (size_t i=0; i<client_no; i++) {
            if (fw_match[i] != -1) continue;
            std::queue<int> q;
            q.push(i);
            int augmenting = -1;
            while(!q.empty()) {
                int cur = q.front();
                q.pop();
                for (auto x: fw_graph[cur]) {
                    if (parent[x] != -1) continue;
                    if (fw_match[cur] == x) continue;
                    parent[x] = cur;
                    if (bw_match[x] == -1) {
                        augmenting = x;
                        break;
                    } else {
                        q.push(bw_match[x]);
                    }
                }
                if (augmenting != -1) break;
            }
            while (augmenting != -1) {
                improved = true;
                int next = fw_match[parent[augmenting]];
                bw_match[augmenting] = parent[augmenting];
                fw_match[parent[augmenting]] = augmenting;
                augmenting = next;
            }
        }
        if (!improved) break;
    }

    std::unordered_map<address, std::pair<address, hash_t>> res;
    for (size_t i=0; i<client_no; i++) {
        if (fw_match[i] == -1) continue;
        address sender = id_to_addr[i];
        address receiver = id_to_addr[fw_match[i]];
        hash_t chunk;
        for (auto& x: clients.at(sender).chunks_owned) {
            if (clients.at(receiver).chunks_needed.count(x) && !clients.at(receiver).chunks_owned.count(x)) {
                chunk = x;
                break;
            }
        }
        res.emplace(sender, std::make_pair(receiver, chunk));
    }
    return res;
}

template<class UI>
void Server<UI>::run() {
    using namespace std::placeholders;
    boost::asio::io_service io_service;

    ui.log("Generating file list...");
    for (auto x = directory_iterator(base_dir); x != directory_iterator(); x++) {
        std::string filename = x->path().filename().string();
        if (!is_regular_file(x->path())) continue;
        ui.log("Found " + filename);
        const std::vector<hash_t>& chunk_list = File(x->path().string()).get_chunk_list();
        files.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(filename),
            std::forward_as_tuple(filename, file_size(*x), chunk_list.begin(), chunk_list.end()));
    }
    ui.log("File list complete!");

    auto send_chunk_sender = [this] (address send, address dest, hash_t chunk, boost::asio::yield_context yield) {
        try {
            send_packet(clients.at(send).socket, yield, SendChunkPacket(dest, chunk));
        } catch (std::exception& e) {
            ui.log("Error sending command: " + std::string(e.what()));
        }
    };

    auto client_manager = [this, &send_chunk_sender] (address addr, boost::asio::yield_context yield) {
        try {
            tcp::socket& socket = clients.at(addr).socket;
            for (;;) {
                packet_type type = get_packet_type(socket, yield);
                switch (type) {
                    case get_file: {
                        GetFilePacket packet(socket, yield);
                        if (!files.count(packet.name)) {
                            send_packet(socket, yield, ErrorPacket(no_such_file));
                        } else  {
                            for (auto& x: files.at(packet.name).chunk_list.chunks) {
                                clients.at(addr).chunks_needed.insert(x);
                            }
                            send_packet(socket, yield, files.at(packet.name));
                        }
                        is_busy.insert(addr);
                        break;
                    }
                    case chunk_list: {
                        ChunkListPacket packet(socket, yield);
                        for (auto& x: packet.chunks) {
                            clients.at(addr).chunks_owned.insert(x);
                        }
                        if (is_busy.count(addr)) {
                            is_busy.erase(is_busy.find(addr));
                        }
                        break;
                    }
                    case new_chunk: {
                        NewChunkPacket packet(socket, yield);
                        clients.at(addr).chunks_owned.insert(packet.chunk);
                        if (is_busy.count(addr)) {
                            is_busy.erase(is_busy.find(addr));
                        }
                        break;
                    }
                    case error: {
                        ui.log("Received error from client: " + ErrorPacket(socket, yield).get_as_string());
                        break;
                    }
                    default: {
                        ui.log("Unknown packet type from client: " + std::to_string(type));
                        ErrorPacket answer(unknown_packet);
                        send_packet(socket, yield, answer);
                    }
                }
                ui.report_client_status(clients);
                if (is_busy.empty()) {
                    for (auto& x: get_chunks_to_send()) {
                        boost::asio::spawn(clients.at(x.first).strand, std::bind(send_chunk_sender, x.first, x.second.first, x.second.second, _1));
                        is_busy.insert(x.second.first);
                    }
                }
            }
        } catch (std::exception& e) {
            try {
                ui.log("Error handling client: " + std::string(e.what()));
                clients.erase(addr);
                if (is_busy.count(addr)) {
                    is_busy.erase(is_busy.find(addr));
                }
                if (is_busy.empty()) {
                    for (auto& x: get_chunks_to_send()) {
                        boost::asio::spawn(clients.at(x.first).strand, std::bind(send_chunk_sender, x.first, x.second.first, x.second.second, _1));
                        is_busy.insert(x.second.first);
                    }
                }
            } catch (std::exception& e) {
                ui.log("Error handling exception: " + std::string(e.what()));
            }
        }
    };

    auto client_connect_listener = [this, &io_service, &client_manager] (boost::asio::yield_context yield) {
        try {
            tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), server_port));
            for (;;) {
                tcp::socket socket(io_service);
                acceptor.async_accept(socket, yield);
                address addr = socket.remote_endpoint().address();
                clients.emplace(addr, std::move(ClientStatus(std::move(socket), io_service)));
                boost::asio::spawn(io_service, std::bind(client_manager, addr, _1));
            }
        } catch (const std::exception& e) {
            ui.log("Client accept: " + std::string(e.what()));
        }
    };

    boost::asio::spawn(io_service, client_connect_listener);
    io_service.run();
}
#endif
