#include "raylib-cpp.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "camera.hpp"
#include "ecs.hpp"
#include "renderer.hpp"

namespace SPRF {

class Script : public Component {
  public:
    void init() { this->entity().get_component<Transform>().position.z = 10; }

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

} // namespace SPRF

int main() {
    raylib::Window window(1024, 1024, "test");

    SPRF::Scene scene;

    auto test = scene.create_entity();
    test->add_component<SPRF::Model>(
        std::make_shared<raylib::Model>(raylib::Mesh::Sphere(1, 50, 50)));
    test->add_component<SPRF::Script>();

    auto child = test->create_child();
    child->add_component<SPRF::Model>(
        std::make_shared<raylib::Model>(raylib::Mesh::Sphere(1, 50, 50)));
    child->get_component<SPRF::Transform>().position.x = 3;

    auto my_camera = scene.create_entity();
    my_camera->add_component<SPRF::Camera>().set_active();
    my_camera->get_component<SPRF::Transform>().position.z = -10;
    my_camera->add_component<SPRF::Script2>();

    scene.init();

    SetTargetFPS(60);

    while (!window.ShouldClose()) {
        scene.draw(window);
    }

    scene.destroy();

    return 0;
}