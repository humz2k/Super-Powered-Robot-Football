#include "networking/server.hpp"
#include "raylib-cpp.hpp"
#include <iostream>
#include <string>
#include <thread>

int main() {
    SPRF::Server server("127.0.0.1", 9999, 4, 2);

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
    return 0;
}