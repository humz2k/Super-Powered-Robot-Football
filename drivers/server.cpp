#include "networking/server.hpp"
#include "raylib-cpp.hpp"

int main() {
    raylib::Window window(300,300,"server");
    SPRF::Networking::Server server("127.0.0.1", 9999, 4, 2);
    while (!window.ShouldClose()){
        server.get_event();
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
    }
    return 0;
}