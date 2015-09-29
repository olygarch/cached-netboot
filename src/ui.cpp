#include "ui.h"
#include <stdio.h>

void BasicUI::print_line(const std::string& line, bool keep) {
    fputs(line.c_str(), stdout);
    if (keep) {
        fputs("\n", stdout);
    } else {
        fputs("\r", stdout);
    }
}

BasicUI::BasicUI(const std::vector<std::string>& header_lines) {
    for (auto& x: header_lines) {
        print_line(x, true);
    }
}

void BasicUI::log(const std::string& message) {
    print_line(message, true);
}

void BasicUI::report_client_status(const std::unordered_map<address, ClientStatus>& clients) {
    size_t clients_done = 0;
    for (auto& x: clients) {
        if (x.second.chunks_owned.size() == x.second.chunks_needed.size()) {
            clients_done++;
        }
    }
    print_line(std::to_string(clients_done) + " of " + std::to_string(clients.size()) + " clients have finished.");
}

void BasicUI::report_status(const std::unordered_map<std::string, File>& files) {
    size_t total_chunks = 0;
    size_t owned_chunks = 0;
    for (auto& x: files) {
        total_chunks += x.second.count_total_chunks();
        owned_chunks += x.second.count_present_chunks();
    }
    print_line("Downloaded " + std::to_string(owned_chunks) + " of " + std::to_string(total_chunks) + " chunks");
}

ANSIUI::ANSIUI(const std::vector<std::string>& header_lines) {
    fputs("\033[2J\033[1;1H", stdout);
    for (auto& l: header_lines) {
        fputs(l.c_str(), stdout);
        fputs("\n", stdout);
    }
    write_ui();
}

void ANSIUI::clear_ui() {
    for (size_t i=0; i<status.size()+logs.size()+1; i++) {
        fputs("\033[1F\033[2L", stdout);
    }
}

void ANSIUI::write_ui() {
    for (auto& x: status) {
        fputs(x.c_str(), stdout);
        fputs("\n", stdout);
    }
    fputs("\n", stdout);
    for (auto& x: logs) {
        fputs(x.c_str(), stdout);
        fputs("\n", stdout);
    }
}

void ANSIUI::report_client_status(const std::unordered_map<address, ClientStatus>& clients) {
    clear_ui();
    std::vector<std::string> status;
    for (auto& x: clients) {
        std::string address = x.first.to_string();
        address.resize(40, ' ');
        status.push_back(address + std::to_string(x.second.chunks_owned.size()) + " of " + std::to_string(x.second.chunks_needed.size()) + " chunks done");
    }
    this->status.swap(status);
    write_ui();
}

void ANSIUI::report_status(const std::unordered_map<std::string, File>& files) {
    clear_ui();
    std::vector<std::string> status;
    for (auto& x: files) {
        std::string filename = x.first;
        filename.resize(40, ' ');
        status.push_back(filename + std::to_string(x.second.count_present_chunks()) + " of " + std::to_string(x.second.count_total_chunks()) + " chunks");
    }
    this->status.swap(status);
    write_ui();
}

void ANSIUI::log(const std::string& message) {
    clear_ui();
    logs.push_back(message);
    if (logs.size() > 10) {
        logs.pop_front();
    }
    write_ui();
}
