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

namespace SPRF{

class TestComp : public Component{
    public:
        void update(){
            if (IsKeyPressed(KEY_J)){
                this->entity()->get_component<Selectable>()->select();
            }
        }
};

class MyScene : public TestScene{
    public:
        MyScene(Game* game) : TestScene(game){
            auto test = this->create_entity();
            auto model = this->renderer()->create_render_model(raylib::Mesh::Sphere(1,10,10));
            test->add_component<Model>(model);
            test->add_component<Selectable>();

            auto test2 = this->create_entity();
            //auto model = this->renderer()->create_render_model(raylib::Mesh::Sphere(1,10,10));
            test2->add_component<Model>(model);
            test2->add_component<Selectable>();
            test2->get_component<Transform>()->position.x -= 1.5;
            test2->add_component<TestComp>();
        }
};

}

int main(){
    assert(enet_initialize() == 0);

    int window_width = 800;
    int window_height = 800;
    int render_width = 800 * 2;
    int render_height = 800 * 2;
    int fps_max = 200;
    int fullscreen = 0;
    float volume = 1.0;

    SPRF::game =
        new SPRF::Game(window_width, window_height, "ik_test", render_width,
                       render_height, fps_max, fullscreen, volume);

    SPRF::game->load_scene<SPRF::MyScene>();

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;
    enet_deinitialize();
    return 0;
}