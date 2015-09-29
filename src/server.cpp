#include "server.h"
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s base_dir\n", argv[0]);
        return 1;
    }
    Server<>(argv[1]).run();
}
