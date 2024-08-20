#include "custom_mesh.hpp"
#include "engine/engine.hpp"
#include <cassert>
#include <string>

namespace SPRF {

class TestScene : public DefaultScene {
  public:
    TestScene(Game* game) : DefaultScene(game) {
        auto light = this->renderer()->add_light();
        light->enabled(1);
        light->L(vec3(1, 2, 0.02));
        light->target(vec3(2.5, 0, 0));
        light->fov(70);

        auto origin = this->create_entity();
        origin->get_component<Transform>()->position.y = 0.5;
        // origin->get_component<Transform>()->position.z = 10;
        auto camera = origin->create_child();
        camera->get_component<Transform>()->position.z = -10;
        camera->get_component<Transform>()->position.y = 0;
        camera->add_component<Camera>();
        camera->get_component<Camera>()->set_active();

        this->renderer()->load_skybox("assets/"
                                      "defaultskybox.png");
        this->renderer()->enable_skybox();
    }
};

} // namespace SPRF

int main() {
    // SetConfigFlags(FLAG_MSAA_4X_HINT);
    //  SPRF::game = new SPRF::Game(1512, 982, "test", 1512 * 2, 982 * 2, 200);
    // SPRF::game = new SPRF::Game(500, 500, "physics test", 500 * 2, 500 * 2,
    // 200);
    //  ToggleFullscreen();

    // SPRF::game->load_scene<SPRF::TestScene>();

    // while (SPRF::game->running()) {
    //     SPRF::game->draw();
    // }

    // delete SPRF::game;
    return 0;
}