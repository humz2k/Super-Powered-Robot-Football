#include "raylib-cpp.hpp"
#include "soloud.h"
#include "soloud_wav.h"

SoLoud::Soloud gSoloud; // SoLoud engine
SoLoud::Wav gWave;

int main() {
    raylib::Window window(900, 900, "test");

    gSoloud.init();

    gWave.load("assets/first_call.wav");

    gSoloud.play(gWave);

    SetTargetFPS(200);

    raylib::Camera3D camera(raylib::Vector3(0, 2, 10), raylib::Vector3(0, 0, 0),
                            raylib::Vector3(0, 1, 0), 45, CAMERA_PERSPECTIVE);

    while (!window.ShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);
        camera.BeginMode();
        DrawGrid(10, 1);
        camera.EndMode();
        EndDrawing();
    }

    gSoloud.deinit();
}