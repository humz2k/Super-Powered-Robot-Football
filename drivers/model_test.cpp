#include "raylib-cpp.hpp"

int main() {
    raylib::Window window(900, 900, "test");

    SetTargetFPS(200);

    raylib::Camera3D camera(raylib::Vector3(0, 2, 10), raylib::Vector3(0, 0, 0),
                            raylib::Vector3(0, 1, 0), 45, CAMERA_PERSPECTIVE);

    raylib::Model model("assets/xbot1.glb");
    int animCount;
    ModelAnimation* anims = LoadModelAnimations("assets/xbot1.glb", &animCount);
    ModelAnimation current_anim = anims[1];
    float current_frame = 0;

    while (!window.ShouldClose()) {
        UpdateModelAnimation(model, current_anim, (int)current_frame);
        current_frame += GetFrameTime() * 30;
        if (((int)current_frame) >= current_anim.frameCount - 1) {
            current_frame = 0;
        }
        BeginDrawing();
        ClearBackground(WHITE);
        camera.BeginMode();
        model.Draw(raylib::Vector3(0, 0, 0), raylib::Vector3(1, 0, 0), 90,
                   raylib::Vector3(0.01, 0.01, 0.01), WHITE);
        DrawGrid(10, 1);
        camera.EndMode();
        EndDrawing();
    }
    UnloadModelAnimations(anims, animCount);
}