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
  private:
    raylib::Vector2 sense = raylib::Vector2(0.005, 0.005);
    float m_speed = 10;
    bool mouse_locked = false;

  public:
    void init() {}

    void update() {
        if (game_info.dev_console_active) {
            if (mouse_locked) {
                EnableCursor();
                mouse_locked = false;
            }
            return;
        }
        if (!mouse_locked) {
            DisableCursor();
            mouse_locked = true;
        }
        auto mouse_delta = raylib::Vector2(GetMouseDelta()) * sense;
        this->entity()->get_component<Transform>()->rotation.x += mouse_delta.y;
        this->entity()->get_component<Transform>()->rotation.y -= mouse_delta.x;

        this->entity()->get_component<Transform>()->rotation.y =
            Wrap(this->entity()->get_component<Transform>()->rotation.y, 0,
                 2 * M_PI);
        this->entity()->get_component<Transform>()->rotation.x =
            Clamp(this->entity()->get_component<Transform>()->rotation.x,
                  -M_PI * 0.5f + 0.5, M_PI * 0.5f - 0.5);

        raylib::Vector3 forward = Vector3Normalize(Vector3RotateByAxisAngle(
            (Vector3){0.0f, 0.0f, 1.0f}, (Vector3){0.0f, 1.0f, 0.0f},
            this->entity()->get_component<Transform>()->rotation.y));
        raylib::Vector3 left = Vector3Normalize(Vector3RotateByAxisAngle(
            (Vector3){1.0f, 0.0f, 0.0f}, (Vector3){0.0f, 1.0f, 0.0f},
            this->entity()->get_component<Transform>()->rotation.y));

        raylib::Vector3 outdir = Vector3Zero();

        if (IsKeyDown(KEY_W)) {
            outdir += forward;
        }
        if (IsKeyDown(KEY_S)) {
            outdir -= forward;
        }
        if (IsKeyDown(KEY_D)) {
            outdir -= left;
        }
        if (IsKeyDown(KEY_A)) {
            outdir += left;
        }

        this->entity()->get_component<Transform>()->position +=
            outdir.Normalize() * (m_speed * game_info.frame_time);

        if (IsKeyPressed(KEY_Q)) {
            this->entity()->scene()->close();
        }
    }
};

class Scene1 : public DefaultScene {
  public:
    Scene1(Game* game) : DefaultScene(game) {
        RenderModel* render_model = this->renderer()->create_render_model(
            raylib::Mesh::Sphere(1, 25, 25));

        // auto test = this->create_entity();
        // test->add_component<SPRF::Model>(render_model);
        // test->add_component<SPRF::Script>();

        for (int i = -20; i < 20; i++) {
            for (int j = -20; j < 20; j++) {
                auto child = this->create_entity();
                child->add_component<SPRF::Model>(render_model);
                // child->add_component<SPRF::FPSController>();
                child->get_component<SPRF::Transform>()->position.x = i * 2;
                child->get_component<SPRF::Transform>()->position.y = 0.5;
                child->get_component<SPRF::Transform>()->position.z = j * 2;
            }
        }

        auto floor = this->create_entity();
        auto floor_model = this->renderer()->create_render_model(
            raylib::Mesh::Plane(20, 20, 10, 10));
        floor_model->clip(false);
        floor->add_component<SPRF::Model>(floor_model);

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

    void on_close() {
        TraceLog(LOG_INFO, "scene closed");
        game()->load_scene<DefaultScene>();
    }
};

class MainMenu : public DefaultScene {
  private:
    raylib::Font m_font;

  public:
    MainMenu(Game* game)
        : DefaultScene(game),
          m_font("/Users/humzaqureshi/GitHub/Super-Powered-Robot-Football/src/"
                 "JetBrainsMono-Regular.ttf",
                 128) {
        set_background_color(BLACK);
        this->create_entity()->add_component<UITextComponent>(
            &m_font, raylib::Vector2(0, 0.01), 0.25,
            "Super\nPowered\nRobot\nFootball");
    }

    void on_close() {}
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