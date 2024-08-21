#include "crosshair.hpp"
#include "custom_mesh.hpp"
#include "editor/editor_tools.hpp"
#include "engine/engine.hpp"
#include "engine/sound.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_raylib.h"
#include "imgui/rlImGui.h"
#include "mouselook.hpp"
#include "networking/client.hpp"
#include "networking/map.hpp"
#include "networking/server.hpp"
#include "testing.hpp"
#include <ik/ik.h>
#include <cassert>
#include <string>

namespace SPRF {

class MyScene : public TestScene {
  public:
    MyScene(Game* game) : TestScene(game, true) {
        find_entity("IMGui Manager")->disable();

        int nx = 50;
        int ny = 50;
        int nz = 50;
        auto model = this->renderer()->create_render_model(Mesh::Sphere(0.25,10,10));

        int count = 0;
        for (int i = 0; i < nx; i++){
            float x = i - (nx/2);
            for (int j = 0; j < ny; j++){
                float y = j - (ny/2);
                for (int k = 0; k < nz; k++){
                    float z = k - (nz/2);
                    auto sphere = this->create_entity("sphere_" + std::to_string(count));
                    sphere->add_component<Model>(model);
                    sphere->get_component<Transform>()->position = vec3(x,y,z);
                    count++;
                }
            }
        }

        dev_console()->exec("assets/editor/cfg/init.cfg");
    }
};

} // namespace SPRF

int main() {
    if(enet_initialize()){
        assert(1 == 0);
    }

    int window_width = 1400;
    int window_height = 900;
    int render_width = 1400 * 2;
    int render_height = 900 * 2;
    int fps_max = 200;
    int fullscreen = 0;
    float volume = 1.0;

    SPRF::game =
        new SPRF::Game(window_width, window_height, "stress_test", render_width,
                       render_height, fps_max, fullscreen, volume);

    rlImGuiSetup(true);

    ik.init();

    SPRF::game->load_scene<SPRF::MyScene>();

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;

    ik.deinit();
    rlImGuiShutdown();
    enet_deinitialize();
    return 0;
}