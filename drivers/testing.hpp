#ifndef _TESTING_SPRF_DRIVERS_HPP_
#define _TESTING_SPRF_DRIVERS_HPP_

#include "crosshair.hpp"
#include "custom_mesh.hpp"
#include "editor/editor_tools.hpp"
#include "engine/engine.hpp"
#include "engine/sound.hpp"
#include "mouselook.hpp"
#include "networking/client.hpp"
#include "networking/map.hpp"
#include "networking/server.hpp"
#include <cassert>
#include <string>

namespace SPRF {

class Rotation : public Component {
  private:
    Transform* transform;
    Entity* m_camera;
    float m_speed = 2;
    bool m_forward = false;
    bool m_backward = false;
    bool m_left = false;
    bool m_right = false;

    void reset_inputs() { m_forward = m_backward = m_left = m_right = false; }

  public:
    Rotation(DevConsole* dev_console) {
        dev_console->add_command<UpdateInput>("+forward", &m_forward);
        dev_console->add_command<UpdateInput>("+backward", &m_backward);
        dev_console->add_command<UpdateInput>("+left", &m_left);
        dev_console->add_command<UpdateInput>("+right", &m_right);
    }
    void init() {
        transform = this->entity()->get_component<Transform>();
        m_camera = this->entity()->get_child(0);
        reset_inputs();
    }
    void update() {
        if (!m_camera->get_component<Camera>()->active()) {
            return;
        }
        if (!IsKeyDown(KEY_Z)) {
            transform->rotation.y -= GetMouseWheelMoveV().x * 0.2;
            transform->rotation.x += GetMouseWheelMoveV().y * 0.2;
        }
        auto cam = *this->entity()->scene()->get_active_camera();
        auto forward =
            (vec3(cam.target) - vec3(cam.position));
        forward.y = 0;
        forward = forward.Normalize();
        auto left =
            vec3(Vector3RotateByAxisAngle(
                                forward, vec3(0, 1, 0), M_PI_2))
                .Normalize();

        transform->position +=
            forward * game_info.frame_time * m_speed * (m_forward - m_backward);
        transform->position +=
            left * game_info.frame_time * m_speed * (m_left - m_right);

        reset_inputs();
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
    TestScene(Game* game, bool create_light = true) : DefaultScene(game) {
        dev_console()->entity()->add_component<Selectable>();

        auto IMGui_Manager = this->create_entity("IMGui Manager");
        IMGui_Manager->add_component<IMGuiManager>();
        IMGui_Manager->add_component<Selectable>();

        if (create_light) {
            auto light = this->renderer()->add_light();
            light->enabled(1);
            light->L(vec3(1, 2, 0.02));
            light->target(vec3(2.5, 0, 0));
            light->fov(70);
        }

        auto origin = this->create_entity("origin");
        origin->add_component<Rotation>(this->dev_console());
        origin->get_component<Transform>()->position.y = 0.5;
        origin->add_component<Selectable>();

        auto camera = origin->create_child("camera");
        camera->add_component<Zoom>();
        camera->get_component<Transform>()->position.z = -10;
        camera->get_component<Transform>()->position.y = 0;
        camera->add_component<Camera>();
        camera->get_component<Camera>()->set_active();
        camera->add_component<Selectable>();

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