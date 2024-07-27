#include "engine.hpp"

namespace SPRF {

class Script : public Component {
  public:
    void init() {
        this->entity()->get_component<Transform>()->position.z = 0;
        this->entity()->get_component<Transform>()->position.x = -2;
        this->entity()->get_component<Transform>()->position.y = 1;
    }

    void update() {
        // this->entity()->get_component<Transform>().position.z -=
        // GetFrameTime();
    }
};

class FPSController : public Component {
  public:
    void init() {}

    void update() {
        if (IsKeyDown(KEY_W)) {
            this->entity()->get_component<Transform>()->position.z +=
                GetFrameTime();
        }
        if (IsKeyDown(KEY_S)) {
            this->entity()->get_component<Transform>()->position.z -=
                GetFrameTime();
        }
        if (IsKeyDown(KEY_A)) {
            this->entity()->get_component<Transform>()->position.x +=
                GetFrameTime();
        }
        if (IsKeyDown(KEY_D)) {
            this->entity()->get_component<Transform>()->position.x -=
                GetFrameTime();
        }
    }
};

class Scene1 : public DefaultScene {
  public:
    Scene1() {
        RenderModel* render_model = this->renderer()->create_render_model(
            raylib::Mesh::Sphere(1, 25, 25));

        // auto test = this->create_entity();
        // test->add_component<SPRF::Model>(render_model);
        // test->add_component<SPRF::Script>();

        for (int i = -10; i < 10; i++) {
            for (int j = -10; j < 10; j++) {
                auto child = this->create_entity();
                child->add_component<SPRF::Model>(render_model);
                // child->add_component<SPRF::FPSController>();
                child->get_component<SPRF::Transform>()->position.x = i * 2;
                child->get_component<SPRF::Transform>()->position.y = 0.5;
                child->get_component<SPRF::Transform>()->position.z = j * 2;
            }
        }

        auto floor = this->create_entity();
        floor->add_component<SPRF::Model>(this->renderer()->create_render_model(
            raylib::Mesh::Plane(20, 20, 10, 10)));

        auto my_camera = this->create_entity();
        my_camera->add_component<SPRF::Camera>()->set_active();
        my_camera->add_component<SPRF::FPSController>();
        my_camera->get_component<SPRF::Transform>()->position.z = -13;
        my_camera->get_component<SPRF::Transform>()->position.y = 5;
        my_camera->get_component<SPRF::Transform>()->rotation.x = 0.4;

        auto light = this->renderer()->add_light();
        light->enabled(1);

        auto light2 = this->renderer()->add_light();
        light2->enabled(1);
        light2->L(raylib::Vector3(0, 2, 5));

        this->renderer()->load_skybox(
            "/Users/humzaqureshi/GitHub/Super-Powered-Robot-Football/src/"
            "defaultskybox.png");
        this->renderer()->disable_skybox();
        this->renderer()->enable_skybox();
    }
};

} // namespace SPRF

int main() {

    SPRF::Game game(900, 900, "test", 900, 900, 500);

    // ToggleFullscreen();

    game.load_scene<SPRF::Scene1>();

    while (game.running()) {
        game.draw();
    }

    return 0;
}