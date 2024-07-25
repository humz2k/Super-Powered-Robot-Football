#include "raylib-cpp.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "camera.hpp"
#include "ecs.hpp"
#include "model.hpp"
#include "shaders.hpp"

#include "game.hpp"

namespace SPRF {

class Script : public Component {
  public:
    void init() {
        this->entity().get_component<Transform>().position.z = 0;
        this->entity().get_component<Transform>().position.x = -2;
        this->entity().get_component<Transform>().position.y = 1;
    }

    void update() {
        // this->entity().get_component<Transform>().position.z -=
        // GetFrameTime();
    }
};

class Script2 : public Component {
  public:
    void init() { this->entity().get_component<Transform>().position.x = 3; }

    void update() {
        // this->entity().get_component<Transform>().rotation.y -=
        // GetFrameTime() * 0.1;
    }
};

class DefaultScene : public Scene {
    public:
        DefaultScene(){
            auto render_model = this->renderer().create_render_model(
            std::make_shared<raylib::Model>(raylib::Mesh::Sphere(1, 50, 50)));

            auto test = this->create_entity();
            test->add_component<SPRF::Model>(render_model);
            test->add_component<SPRF::Script>();

            auto child = this->create_entity();
            child->add_component<SPRF::Model>(render_model);
            child->get_component<SPRF::Transform>().position.x = 2;
            child->get_component<SPRF::Transform>().position.y = 1;

            auto floor = this->create_entity();
            floor->add_component<SPRF::Model>(this->renderer().create_render_model(
                std::make_shared<raylib::Model>(raylib::Mesh::Plane(20, 20, 10, 10))));

            auto my_camera = this->create_entity();
            my_camera->add_component<SPRF::Camera>().set_active();
            my_camera->get_component<SPRF::Transform>().position.z = -13;
            my_camera->get_component<SPRF::Transform>().position.y = 2;
            my_camera->get_component<SPRF::Transform>().rotation.x = 0.1;

            auto light = this->renderer().add_light();
            light->enabled(1);

            auto light2 = this->renderer().add_light();
            light2->enabled(1);
            light2->L(raylib::Vector3(0, 2, 5));
        }
};

} // namespace SPRF

int main() {

    SPRF::Game game(1184,666,"test",1024,768);

    game.load_scene<SPRF::DefaultScene>();

    while(game.running()){
        game.draw();
    }

    return 0;
}