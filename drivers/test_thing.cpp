#include "custom_mesh.hpp"
#include "engine/engine.hpp"
#include <cassert>
#include <string>

namespace SPRF {

class Rotation : public Component {
  private:
    Transform* transform;
    float m_speed = 2;

  public:
    void init() { transform = this->entity()->get_component<Transform>(); }
    void update() {
        if (!IsKeyDown(KEY_Z)) {
            transform->rotation.y -= GetMouseWheelMoveV().x * 0.2;
            transform->rotation.x += GetMouseWheelMoveV().y * 0.2;
        }
        float speed = m_speed;
        if (IsKeyDown(KEY_W)) {
            transform->position.z += game_info.frame_time * speed;
        }
        if (IsKeyDown(KEY_S)) {
            transform->position.z -= game_info.frame_time * speed;
        }
        if (IsKeyDown(KEY_A)) {
            transform->position.x += game_info.frame_time * speed;
        }
        if (IsKeyDown(KEY_D)) {
            transform->position.x -= game_info.frame_time * speed;
        }
        // if (!game_info.dev_console_active){
        //     DisableCursor();
        // } else {
        //     EnableCursor();
        // }
    }
};

class Zoom : public Component {
  public:
    void update() {
        if (IsKeyDown(KEY_Z)) {
            this->entity()->get_component<Transform>()->position.z +=
                GetMouseWheelMoveV().y;
            if (this->entity()->get_component<Transform>()->position.z > -1) {
                this->entity()->get_component<Transform>()->position.z = -1;
            }
        }
    }
};

class TestScene : public DefaultScene {
  public:
    TestScene(Game* game) : DefaultScene(game) {
        auto light = this->renderer()->add_light();
        light->enabled(1);
        light->L(raylib::Vector3(1, 2, 0.02));
        light->target(raylib::Vector3(2.5, 0, 0));
        light->fov(70);

        auto origin = this->create_entity();
        origin->add_component<Rotation>();
        origin->get_component<Transform>()->position.y = 0.5;
        // origin->get_component<Transform>()->position.z = 10;
        auto camera = origin->create_child();
        camera->add_component<Zoom>();
        camera->get_component<Transform>()->position.z = -10;
        camera->get_component<Transform>()->position.y = 0;
        camera->add_component<Camera>();
        camera->get_component<Camera>()->set_active();

        this->renderer()->load_skybox("assets/"
                                      "defaultskybox.png");
        this->renderer()->enable_skybox();

        int map_x_size = 70;
        int map_z_size = 60;

        auto plane = this->renderer()->create_render_model(
            WrappedMesh(map_x_size, map_z_size, 10, 10));
        plane->add_texture("assets/prototype_texture/grey4.png");
        plane->clip(false);
        auto plane_entity = this->create_entity();
        plane_entity->add_component<Model>(plane);
        // plane_entity->get_component<Transform>()->position.z = 10;

        auto cube =
            this->renderer()->create_render_model(raylib::Mesh::Cube(1, 1, 1));
        cube->add_texture("assets/prototype_texture/orange-cube.png");

        auto sphere = this->renderer()->create_render_model(
            raylib::Mesh::Sphere(0.5, 30, 30));
        // cube->add_texture("assets/prototype_texture/orange-cube.png");

        auto sphere1 = this->create_entity();
        sphere1->get_component<Transform>()->position.y = 0.5;
        sphere1->get_component<Transform>()->position.z = 0.5; // + (float)10;
        sphere1->get_component<Transform>()->rotation.y = 0;
        sphere1->add_component<Model>(sphere);

        auto sphere2 = this->create_entity();
        sphere2->get_component<Transform>()->position.y = 0.5;
        sphere2->get_component<Transform>()->position.z = 0.5 + (float)2;
        sphere2->get_component<Transform>()->rotation.y = M_PI_2;
        sphere2->add_component<Model>(sphere);

        for (int i = -(map_z_size / 2); i < (map_z_size / 2); i++) {
            for (int y = 0; y < 5; y++) {
                auto cube_entity = this->create_entity();
                cube_entity->get_component<Transform>()->position.y =
                    0.5 + (float)y;
                cube_entity->get_component<Transform>()->position.z =
                    0.5 + (float)i;
                cube_entity->get_component<Transform>()->position.x =
                    0.5 + -((float)map_x_size) * 0.5;
                cube_entity->add_component<Model>(cube);

                auto cube_entity2 = this->create_entity();
                cube_entity2->get_component<Transform>()->position.y =
                    0.5 + (float)y;
                cube_entity2->get_component<Transform>()->position.z =
                    0.5 + (float)i;
                cube_entity2->get_component<Transform>()->position.x =
                    0.5 + ((float)map_x_size) * 0.5;
                cube_entity2->add_component<Model>(cube);
            }
        }

        for (int i = -(map_x_size / 2) + 1; i < (map_x_size / 2); i++) {
            for (int y = 0; y < 5; y++) {
                auto cube_entity = this->create_entity();
                cube_entity->get_component<Transform>()->position.y =
                    0.5 + (float)y;
                cube_entity->get_component<Transform>()->position.x =
                    0.5 + (float)i;
                cube_entity->get_component<Transform>()->position.z =
                    0.5 + -((float)map_z_size / 2);
                cube_entity->add_component<Model>(cube);

                auto cube_entity2 = this->create_entity();
                cube_entity2->get_component<Transform>()->position.y =
                    0.5 + (float)y;
                cube_entity2->get_component<Transform>()->position.x =
                    0.5 + (float)i;
                cube_entity2->get_component<Transform>()->position.z =
                    0.5 + ((float)map_z_size / 2) - 1.0;
                cube_entity2->add_component<Model>(cube);
            }
        }
    }
};
} // namespace SPRF

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    // SPRF::game = new SPRF::Game(1512, 982, "test", 1512 * 2, 982 * 2, 200);
    SPRF::game = new SPRF::Game(982, 982, "test", 982 * 2, 982 * 2, 500);
    // ToggleFullscreen();

    SPRF::game->load_scene<SPRF::TestScene>();

    while (SPRF::game->running()) {
        SPRF::game->draw();
    }

    delete SPRF::game;
    return 0;
}