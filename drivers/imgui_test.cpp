/*******************************************************************************************
 *
 *   raylib-extras [ImGui] example - Simple Integration
 *
 *	This is a simple ImGui Integration
 *	It is done using C++ but with C style code
 *	It can be done in C as well if you use the C ImGui wrapper
 *	https://github.com/cimgui/cimgui
 *
 *   Copyright (c) 2021 Jeffery Myers
 *
 ********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

#include "imgui/imgui.h"
#include "imgui/rlImGui.h"

int main(int argc, char* argv[]) {
    // Initialization
    //--------------------------------------------------------------------------------------
    int screenWidth = 1280;
    int screenHeight = 800;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight,
               "raylib-Extras [ImGui] example - simple ImGui Demo");
    SetTargetFPS(144);
    rlImGuiSetup(true);

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        BeginDrawing();
        rlImGuiBegin();
        ClearBackground(DARKGRAY);

        // start ImGui Conent

        // show ImGui Content
        bool open = true;
        ImGui::ShowDemoWindow(&open);

        // end ImGui Content
        rlImGuiEnd();
        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    rlImGuiShutdown();

    // De-Initialization
    //--------------------------------------------------------------------------------------
    rlCloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}