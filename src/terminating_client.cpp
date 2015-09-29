#include "client.h"
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s server_ip base_dir file [file [file ...]]\n", argv[0]);
        return 1;
    }
    std::vector<std::string> files;
    for (int i=3; i<argc; i++) {
        files.push_back(argv[i]);
    }
    Client<>(address::from_string(argv[1]), argv[2], files).run_until_complete();
}
