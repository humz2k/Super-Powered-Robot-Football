#ifndef _SPRB_ECS_HPP_
#define _SPRB_ECS_HPP_

#include <iostream>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <cassert>
#include "raylib-cpp.hpp"

namespace SPRF{

class Entity;
class Scene;
class Component;
class Transform;
class Camera;

class Component{
    private:
        Entity* m_entity = NULL;
    protected:
        Entity& entity(){return *m_entity;}
    public:
        Component(){ }
        void set_parent(Entity* entity_){
            m_entity = entity_;
        }
        virtual ~Component(){}
        virtual void init(){}
        virtual void destroy(){}
        virtual void update(){}
        virtual void draw3D(raylib::Matrix transform){}
        virtual void draw2D(){}
};

class Transform{
    public:
        raylib::Vector3 position = raylib::Vector3(0,0,0);
        raylib::Vector3 rotation = raylib::Vector3(0,0,0);
        raylib::Vector3 scale = raylib::Vector3(1,1,1);

        Transform(raylib::Vector3 position_ = raylib::Vector3(0,0,0), raylib::Vector3 rotation_ = raylib::Vector3(0,0,0), raylib::Vector3 scale_ = raylib::Vector3(1,1,1))
        : position(position_), rotation(rotation_), scale(scale_){}

        raylib::Matrix matrix(){
            auto [rotationAxis,rotationAngle] = raylib::Quaternion::FromEuler(rotation).ToAxisAngle();
            auto mat_scale = raylib::Matrix::Scale(scale.x,scale.y,scale.z);
            auto mat_rotation = raylib::Matrix::Rotate(rotationAxis,rotationAngle);
            auto mat_translation = raylib::Matrix::Translate(position.x,position.y,position.z);
            return (mat_scale * mat_rotation) * mat_translation;
        }
};

class Entity{
    private:
        std::unordered_map<std::type_index,Component*> m_components;
        std::vector<std::shared_ptr<Entity>> m_children;
        Transform m_transform;
        Scene& m_scene;
        Entity* m_parent = NULL;

        void add_child(std::shared_ptr<Entity> child){
            m_children.push_back(child);
        }

        void draw3D(raylib::Matrix parent_transform){
            raylib::Matrix transform = parent_transform * m_transform.matrix();
            for (const auto & [ key, value ] : m_components) {
                value->draw3D(transform);
            }
            for (auto i : m_children){
                i->draw3D(transform);
            }
        }
    public:
        Entity(Scene& scene) : m_scene(scene){};

        Entity(Scene& scene, Entity* parent) : m_scene(scene), m_parent(parent){};

        ~Entity(){
            for (const auto & [ key, value ] : m_components) {
                delete value;
                m_components.erase(key);
            }
        }

        raylib::Matrix global_transform(){
            if (!m_parent){
                return m_transform.matrix();
            }
            return m_parent->global_transform() * m_transform.matrix();
        }

        std::shared_ptr<Entity> create_child(){
            auto out = std::make_shared<Entity>(m_scene,this);
            add_child(out);
            return out;
        }

        void update(){
            for (const auto & [ key, value ] : m_components) {
                value->update();
            }
            for (auto i : m_children){
                i->update();
            }
        }

        void draw3D(){
            raylib::Matrix transform = m_transform.matrix();
            for (const auto & [ key, value ] : m_components) {
                value->draw3D(transform);
            }
            for (auto i : m_children){
                i->draw3D(transform);
            }
        }

        void draw2D(){
            for (const auto & [ key, value ] : m_components) {
                value->draw2D();
            }
            for (auto i : m_children){
                i->draw2D();
            }
        }

        void init(){
            for (const auto & [ key, value ] : m_components) {
                value->init();
            }
            for (auto i : m_children){
                i->init();
            }
        }

        void destroy(){
            for (const auto & [ key, value ] : m_components) {
                value->destroy();
            }
            for (auto i : m_children){
                i->destroy();
            }
        }

        template<class T, typename... Args>
        T& add_component(Args... args){
            auto temp = m_components.find(std::type_index(typeid(T)));
            assert((temp == m_components.end()));
            m_components[std::type_index(typeid(T))] = new T(args...);
            m_components[std::type_index(typeid(T))]->set_parent(this);
            return get_component<T>();
        }

        template<class T>
        T& get_component(){
            auto temp = m_components.find(std::type_index(typeid(T)));
            assert((temp != m_components.end()));
            T& component = *dynamic_cast<T*>(temp->second);
            return component;
        }

        template <>
        Transform& get_component<Transform>(){
            return m_transform;
        }

        Scene& scene(){
            return m_scene;
        }
};

class Scene{
    private:
        std::vector<std::shared_ptr<Entity>> m_entities;

        void add_entity(std::shared_ptr<Entity> entity){
            m_entities.push_back(entity);
        }

        raylib::Camera3D* m_active_camera = NULL;
    public:
        Scene(){}

        std::shared_ptr<Entity> create_entity(){
            auto out = std::make_shared<Entity>(*this);
            add_entity(out);
            return out;
        }

        void init(){
            for (auto i : m_entities){
                i->init();
            }
        }

        void update(){
            for (auto i : m_entities){
                i->update();
            }
        }

        void draw3D(){
            for (auto i : m_entities){
                i->draw3D();
            }
        }

        void draw2D(){
            for (auto i : m_entities){
                i->draw2D();
            }
        }

        void destroy(){
            for (auto i : m_entities){
                i->destroy();
            }
        }

        void set_active_camera(raylib::Camera3D* camera){
            m_active_camera = camera;
        }

        raylib::Camera3D& get_active_camera(){
            assert(m_active_camera);
            return *m_active_camera;
        }
};

class Camera : public Component{
    private:
        raylib::Camera3D m_camera;
    public:
        Camera() : m_camera(raylib::Vector3(0,0,0),raylib::Vector3(0,0,0),raylib::Vector3(0,1,0),45.0f,CAMERA_PERSPECTIVE){

        }

        void set_active(){
            this->entity().scene().set_active_camera(&m_camera);
        }

        void update(){
            auto transform = this->entity().global_transform();
            auto feet = raylib::Vector3(0,0,0).Transform(transform);
            auto head = raylib::Vector3(0,1,0).Transform(transform);
            auto eyes = raylib::Vector3(0,0,1).Transform(transform);
            m_camera.SetPosition(feet);
            m_camera.SetUp((head - feet).Normalize());
            m_camera.SetTarget(eyes);
        }
};

}

#endif //_SPRB_ECS_HPP_