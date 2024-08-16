#include "engine/engine.hpp"
#include "networking/client.hpp"
#include "networking/map.hpp"
#include "networking/server.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

static bool file_exists(std::string fileName) {
    std::ifstream infile(fileName);
    return infile.good();
}

int main(int argc, char** argv) {
    if (argc != 2) {
        TraceLog(LOG_ERROR, "Usage: ./server <config_file>");
        return 1;
    }
    std::string config_file = std::string(argv[1]);
    if (!file_exists(config_file)) {
        TraceLog(LOG_ERROR, "Couldn't open config file: %s",
                 config_file.c_str());
        return 1;
    }
    assert(enet_initialize() == 0);
    SPRF::Server server(config_file);

    while (true) {
        std::string command;
        std::cout << "> ";
        std::getline(std::cin, command);
        if (command == "quit") {
            server.quit();
            break;
        }
    }

    server.join();
    enet_deinitialize();
    return 0;
}