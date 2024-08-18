#ifndef _TESTING_SPRF_DRIVERS_HPP_
#define _TESTING_SPRF_DRIVERS_HPP_

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

namespace SPRF{


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

        auto camera = origin->create_child();
        camera->add_component<Zoom>();
        camera->get_component<Transform>()->position.z = -10;
        camera->get_component<Transform>()->position.y = 0;
        camera->add_component<Camera>();
        camera->get_component<Camera>()->set_active();

        this->renderer()->load_skybox("assets/"
                                      "defaultskybox.png");
        this->renderer()->enable_skybox();

        /*auto plane = this->renderer()->create_render_model(
            WrappedMesh(10, 10, 10, 10));
        plane->add_texture("assets/prototype_texture/grey4.png");
        plane->clip(false);
        auto plane_entity = this->create_entity();
        plane_entity->add_component<Model>(plane);*/
    }
};

} // namespace SPRF

#endif // _TESTING_SPRF_DRIVERS_HPP_