#include "raylib-cpp.hpp"
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "ecs.hpp"

namespace SPRF{

class Model : public Component{
    private:
        std::shared_ptr<raylib::Model> m_model;
    public:
        Model(std::shared_ptr<raylib::Model> model) : m_model(model){ }

        void draw3D(raylib::Matrix parent_transform){
            raylib::Matrix transform = raylib::Matrix(m_model->GetTransform()) * parent_transform;
            for (int i = 0; i < m_model->meshCount; i++){
                DrawMesh(m_model->meshes[i],m_model->materials[m_model->meshMaterial[i]], transform);
            }
        }
};

class Script : public Component{
    public:
        void init(){
            this->entity().get_component<Transform>().position.z = 10;
        }

        void update(){
            //this->entity().get_component<Transform>().position.z -= GetFrameTime();
        }
};

class Script2 : public Component{
    public:
        void init(){
            this->entity().get_component<Transform>().position.x = 3;
        }

        void update(){
            //this->entity().get_component<Transform>().rotation.y -= GetFrameTime() * 0.1;
        }
};

}


int main(){
    raylib::Window window(1024,1024,"test");

    SPRF::Scene scene;

    auto test = scene.create_entity();
    test->add_component<SPRF::Model>(std::make_shared<raylib::Model>(raylib::Mesh::Sphere(1,50,50)));
    test->add_component<SPRF::Script>();

    auto child = test->create_child();
    child->add_component<SPRF::Model>(std::make_shared<raylib::Model>(raylib::Mesh::Sphere(1,50,50)));
    child->get_component<SPRF::Transform>().position.x = 3;

    auto my_camera = scene.create_entity();
    my_camera->add_component<SPRF::Camera>().set_active();
    my_camera->get_component<SPRF::Transform>().position.z = -10;
    my_camera->add_component<SPRF::Script2>();

    scene.init();

    SetTargetFPS(60);

    while (!window.ShouldClose()){

        scene.update();

        BeginDrawing();

        window.ClearBackground(BLACK);

        scene.get_active_camera().BeginMode();

        scene.draw3D();

        scene.get_active_camera().EndMode();

        scene.draw2D();

        DrawFPS(10,10);

        EndDrawing();
    }

    scene.destroy();

    return 0;
}