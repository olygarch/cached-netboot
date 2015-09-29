#ifndef CN_UI_H
#define CN_UI_H
#include <vector>
#include <deque>
#include "file.h"

class BasicUI {
    void print_line(const std::string& line, bool keep=false);
public:
    BasicUI(const std::vector<std::string>& header_lines);
    void report_client_status(const std::unordered_map<address, ClientStatus>& clients);
    void report_status(const std::unordered_map<std::string, File>& files);
    void log(const std::string& message);
};

class ANSIUI {
    void clear_ui();
    void write_ui();
    std::vector<std::string> status;
    std::deque<std::string> logs;
public:
    ANSIUI(const std::vector<std::string>& header_lines);
    void report_client_status(const std::unordered_map<address, ClientStatus>& clients);
    void report_status(const std::unordered_map<std::string, File>& files);
    void log(const std::string& message);
};

#ifdef _WIN32
typedef BasicUI DefaultUI;
#else
typedef ANSIUI DefaultUI;
#endif

#endif
