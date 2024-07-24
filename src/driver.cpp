#include "raylib-cpp.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "camera.hpp"
#include "ecs.hpp"
#include "model.hpp"
#include "shaders.hpp"

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

} // namespace SPRF

int main() {
    raylib::Window window(900, 900, "test");

    SPRF::Scene scene;

    auto render_model = scene.renderer().create_render_model(
        std::make_shared<raylib::Model>(raylib::Mesh::Sphere(1, 50, 50)));

    auto test = scene.create_entity();
    test->add_component<SPRF::Model>(render_model);
    test->add_component<SPRF::Script>();

    auto child = scene.create_entity();
    child->add_component<SPRF::Model>(render_model);
    child->get_component<SPRF::Transform>().position.x = 2;
    child->get_component<SPRF::Transform>().position.y = 1;

    auto floor = scene.create_entity();
    floor->add_component<SPRF::Model>(scene.renderer().create_render_model(
        std::make_shared<raylib::Model>(raylib::Mesh::Plane(20, 20, 10, 10))));

    auto my_camera = scene.create_entity();
    my_camera->add_component<SPRF::Camera>().set_active();
    my_camera->get_component<SPRF::Transform>().position.z = -13;
    my_camera->get_component<SPRF::Transform>().position.y = 2;
    my_camera->get_component<SPRF::Transform>().rotation.x = 0.1;
    //my_camera->add_component<SPRF::Script2>();

    auto light = scene.renderer().add_light();
    light->enabled(1);

    auto light2 = scene.renderer().add_light();
    light2->enabled(1);
    light2->L(raylib::Vector3(0,2,5));

    scene.init();

    SetTargetFPS(60);

    raylib::RenderTexture2D tex(1024, 1024);
    raylib::Rectangle render_rect(-raylib::Vector2(tex.GetTexture().GetSize()));
    render_rect.SetWidth(-render_rect.GetWidth());

    while (!window.ShouldClose()) {
        BeginDrawing();
        scene.draw(tex);

        raylib::Rectangle window_rect(window.GetSize());

        tex.GetTexture().Draw(render_rect, window_rect);
        EndDrawing();
    }

    scene.destroy();

    return 0;
}