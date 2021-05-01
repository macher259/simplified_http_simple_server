#include <iostream>
#include <regex>
#include "debug.h"
#include "TCPServer.h"

constexpr int MIN_ARGC = 3;
constexpr int MAX_ARGC = 4;
constexpr size_t DIRECTORY_INDEX = 1;
constexpr size_t CORRELATED_INDEX = 2;
constexpr size_t PORT_NUMBER = 3;

namespace {
    void check_argc(int argc) {
        if (argc < MIN_ARGC || argc > MAX_ARGC) {
            debug("Bad number of arguments.");
            exit(EXIT_FAILURE);
        }
    }

    void check_port(const std::string &port) {
        const static std::regex port_regex(R"(^\d+$)");
        std::smatch match;
        bool matched = std::regex_match(port, match, port_regex);
        if (!matched) {
            debug("Bad port number.");
            exit(EXIT_FAILURE);
        }
    }

}

int main(int argc, char *argv[]) {
    check_argc(argc);
    int port = 8080;
    if (argc == MAX_ARGC) {
        check_port(argv[PORT_NUMBER]);
        port = std::atoi(argv[PORT_NUMBER]);
    }

    try {
        TCPServer server(argv[DIRECTORY_INDEX], argv[CORRELATED_INDEX], port);
        server.run();
    } catch (...) {
        exit(EXIT_FAILURE);
    }

    return 0;
}
