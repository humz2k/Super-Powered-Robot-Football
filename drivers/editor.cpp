#include "crosshair.hpp"
#include "custom_mesh.hpp"
#include "engine/engine.hpp"
#include "engine/sound.hpp"
#include "mouselook.hpp"
#include "networking/client.hpp"
#include "networking/map.hpp"
#include "networking/server.hpp"
#include <cassert>
#include <string>
#include "testing.hpp"
#include "editor/editor_tools.hpp"
#include "imgui/imgui.h"
#include "imgui/rlImGui.h"
#include "imgui/imgui_impl_raylib.h"

namespace SPRF{

class MyScene : public TestScene{
    public:
        MyScene(Game* game) : TestScene(game,false){
            simple_map()->load_editor(this);
        }
};

}

int main(){
    assert(enet_initialize() == 0);

    int window_width = 1400;
    int window_height = 900;
    int render_width = 1400 * 2;
    int render_height = 900 * 2;
    int fps_max = 200;
    int fullscreen = 0;
    float volume = 1.0;

    SPRF::game =
        new SPRF::Game(window_width, window_height, "ik_test", render_width,
                       render_height, fps_max, fullscreen, volume);

    rlImGuiSetup(true);

    SPRF::game->load_scene<SPRF::MyScene>();

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;
    //ImGui::PopFont();
    rlImGuiShutdown();
    enet_deinitialize();
    return 0;
}