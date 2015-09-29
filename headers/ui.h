#ifndef CN_UI_H
#define CN_UI_H
#include <vector>
#include "file.h"

class BasicUI {
    void print_line(const std::string& line, bool keep=false);
public:
    BasicUI(const std::vector<std::string>& header_lines);
    void report_client_status(const std::unordered_map<address, ClientStatus>& clients);
    void report_status(const std::unordered_map<std::string, File>& files);
};

class ANSIUI {
    void print_lines(const std::vector<std::string>& lines);
    size_t last_line_no;
public:
    ANSIUI(const std::vector<std::string>& header_lines);
    void report_client_status(const std::unordered_map<address, ClientStatus>& clients);
    void report_status(const std::unordered_map<std::string, File>& files);
};

#ifdef _WIN32
typedef BasicUI DefaultUI;
#else
typedef ANSIUI DefaultUI;
#endif

#endif
