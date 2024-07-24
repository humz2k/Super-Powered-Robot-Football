#ifndef _SPRB_ECS_HPP_
#define _SPRB_ECS_HPP_

#include "raylib-cpp.hpp"
#include "renderer.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace SPRF {

class Entity;
class Scene;
class Component;
class Transform;

/**
 * @brief Base class for all components.
 */
class Component {
  private:
    /** @brief Pointer to the parent entity */
    Entity* m_entity = NULL;

  protected:
    /**
     * @brief Get the parent entity of the component.
     * @return Reference to the parent entity.
     */
    Entity& entity() { return *m_entity; }

  public:
    Component() {}
    /**
     * @brief Set the parent entity of the component.
     * @param entity_ Pointer to the parent entity.
     */
    void set_parent(Entity* entity_) { m_entity = entity_; }
    virtual ~Component() {}
    /**
     * @brief Initialize the component.
     */
    virtual void init() {}
    /**
     * @brief Destroy the component.
     */
    virtual void destroy() {}
    /**
     * @brief Update the component.
     */
    virtual void update() {}
    /**
     * @brief Update the component before update.
     */
    virtual void before_update() {}
    /**
     * @brief Update the component after update.
     */
    virtual void after_update() {}
    /**
     * @brief Draw3D.
     * @param transform Transformation matrix for drawing.
     */
    virtual void draw3D(raylib::Matrix transform) {}
    /**
     * @brief Draw2D.
     */
    virtual void draw2D() {}
};

/**
 * @brief Class representing a transform with position, rotation, and scale.
 */
class Transform {
  public:
    /** @brief Position of the transform */
    raylib::Vector3 position;
    /** @brief Rotation of the transform */
    raylib::Vector3 rotation;
    /** @brief Scale of the transform */
    raylib::Vector3 scale;

    /**
     * @brief Construct a new Transform object.
     * @param position_ Initial position.
     * @param rotation_ Initial rotation.
     * @param scale_ Initial scale.
     */
    Transform(raylib::Vector3 position_ = raylib::Vector3(0, 0, 0),
              raylib::Vector3 rotation_ = raylib::Vector3(0, 0, 0),
              raylib::Vector3 scale_ = raylib::Vector3(1, 1, 1))
        : position(position_), rotation(rotation_), scale(scale_) {}

    /**
     * @brief Get the transformation matrix.
     * @return Transformation matrix.
     */
    raylib::Matrix matrix() {
        auto [rotationAxis, rotationAngle] =
            raylib::Quaternion::FromEuler(rotation).ToAxisAngle();
        auto mat_scale = raylib::Matrix::Scale(scale.x, scale.y, scale.z);
        auto mat_rotation = raylib::Matrix::Rotate(rotationAxis, rotationAngle);
        auto mat_translation =
            raylib::Matrix::Translate(position.x, position.y, position.z);
        return (mat_scale * mat_rotation) * mat_translation;
    }
};

/**
 * @brief Class representing an entity in the scene.
 */
class Entity {
  private:
    /** @brief Components attached to the entity */
    std::unordered_map<std::type_index, Component*> m_components;
    /** @brief Children entities */
    std::vector<std::shared_ptr<Entity>> m_children;
    /** @brief Transform of the entity */
    Transform m_transform;
    /** @brief Reference to the scene */
    Scene& m_scene;
    /** @brief Pointer to the parent entity */
    Entity* m_parent = NULL;

    /**
     * @brief Add a child entity.
     * @param child Shared pointer to the child entity.
     */
    void add_child(std::shared_ptr<Entity> child) {
        m_children.push_back(child);
    }

    /**
     * @brief Call draw3D on components.
     * @param parent_transform Transformation matrix of the parent.
     */
    void draw3D(raylib::Matrix parent_transform) {
        raylib::Matrix transform = parent_transform * m_transform.matrix();
        for (const auto& [key, value] : m_components) {
            value->draw3D(transform);
        }
        for (auto i : m_children) {
            i->draw3D(transform);
        }
    }

    /**
     * @brief Call after_update on components.
     */
    void after_update() {
        for (const auto& [key, value] : m_components) {
            value->after_update();
        }
        for (auto i : m_children) {
            i->after_update();
        }
    }

    /**
     * @brief Call before_update on components.
     */
    void before_update() {
        for (const auto& [key, value] : m_components) {
            value->before_update();
        }
        for (auto i : m_children) {
            i->before_update();
        }
    }

  public:
    /**
     * @brief Construct a new Entity object.
     * @param scene Reference to the scene.
     */
    Entity(Scene& scene) : m_scene(scene){};

    /**
     * @brief Construct a new Entity object with a parent entity.
     * @param scene Reference to the scene.
     * @param parent Pointer to the parent entity.
     */
    Entity(Scene& scene, Entity* parent) : m_scene(scene), m_parent(parent){};

    /**
     * @brief Destroy the Entity object and its components.
     */
    ~Entity() {
        for (const auto& [key, value] : m_components) {
            delete value;
            m_components.erase(key);
        }
    }

    /**
     * @brief Get the global transformation matrix.
     * @return Global transformation matrix.
     */
    raylib::Matrix global_transform() {
        if (!m_parent) {
            return m_transform.matrix();
        }
        return m_parent->global_transform() * m_transform.matrix();
    }

    /**
     * @brief Create a child entity.
     * @return Shared pointer to the child entity.
     */
    std::shared_ptr<Entity> create_child() {
        auto out = std::make_shared<Entity>(m_scene, this);
        add_child(out);
        return out;
    }

    /**
     * @brief Call update on components.
     */
    void update() {
        before_update();
        for (const auto& [key, value] : m_components) {
            value->update();
        }
        for (auto i : m_children) {
            i->update();
        }
        after_update();
    }

    /**
     * @brief Call draw3D on components.
     */
    void draw3D() {
        raylib::Matrix transform = m_transform.matrix();
        for (const auto& [key, value] : m_components) {
            value->draw3D(transform);
        }
        for (auto i : m_children) {
            i->draw3D(transform);
        }
    }

    /**
     * @brief Call draw2D on components.
     */
    void draw2D() {
        for (const auto& [key, value] : m_components) {
            value->draw2D();
        }
        for (auto i : m_children) {
            i->draw2D();
        }
    }

    /**
     * @brief Call init on components.
     */
    void init() {
        for (const auto& [key, value] : m_components) {
            value->init();
        }
        for (auto i : m_children) {
            i->init();
        }
    }

    /**
     * @brief Call destroy on components.
     */
    void destroy() {
        for (const auto& [key, value] : m_components) {
            value->destroy();
        }
        for (auto i : m_children) {
            i->destroy();
        }
    }

    /**
     * @brief Add a component to the entity.
     * @tparam T Type of the component.
     * @tparam Args Parameter pack for the component constructor.
     * @param args Arguments for the component constructor.
     * @return Reference to the added component.
     */
    template <class T, typename... Args> T& add_component(Args... args) {
        auto temp = m_components.find(std::type_index(typeid(T)));
        assert((temp == m_components.end()));
        m_components[std::type_index(typeid(T))] = new T(args...);
        m_components[std::type_index(typeid(T))]->set_parent(this);
        return get_component<T>();
    }

    /**
     * @brief Get a component of the entity.
     * @tparam T Type of the component.
     * @return Reference to the component.
     */
    template <class T> T& get_component() {
        auto temp = m_components.find(std::type_index(typeid(T)));
        assert((temp != m_components.end()));
        T& component = *dynamic_cast<T*>(temp->second);
        return component;
    }

    /**
     * @brief Specialization for getting the Transform component.
     * @return Reference to the Transform component.
     */
    template <> Transform& get_component<Transform>() { return m_transform; }

    /**
     * @brief Get the scene the entity belongs to.
     * @return Reference to the scene.
     */
    Scene& scene() { return m_scene; }
};

/**
 * @brief Class representing a scene containing entities.
 */
class Scene {
  private:
    /** @brief Entities in the scene */
    std::vector<std::shared_ptr<Entity>> m_entities;

    /** @brief Active camera in the scene */
    raylib::Camera3D* m_active_camera = NULL;

    Renderer m_renderer;

    /**
     * @brief Add an entity to the scene.
     * @param entity Shared pointer to the entity.
     */
    void add_entity(std::shared_ptr<Entity> entity) {
        m_entities.push_back(entity);
    }

    /**
     * @brief Update entity components.
     */
    void update() {
        for (auto i : m_entities) {
            i->update();
        }
    }

    /**
     * @brief Call draw3D on entity components.
     */
    void draw3D() {
        for (auto i : m_entities) {
            i->draw3D();
        }
    }

    /**
     * @brief Call draw2D on entity components.
     */
    void draw2D() {
        for (auto i : m_entities) {
            i->draw2D();
        }
    }

    /**
     * @brief Get the active camera of the scene.
     * @return Reference to the active camera.
     */
    raylib::Camera3D& get_active_camera() {
        assert(m_active_camera);
        return *m_active_camera;
    }

  public:
    Scene() {
    }

    Renderer& renderer() { return m_renderer; }

    /**
     * @brief Create a new entity in the scene.
     * @return Shared pointer to the created entity.
     */
    std::shared_ptr<Entity> create_entity() {
        auto out = std::make_shared<Entity>(*this);
        add_entity(out);
        return out;
    }

    /**
     * @brief Initialize entity components.
     */
    void init() {
        for (auto i : m_entities) {
            i->init();
        }
    }

    /**
     * @brief Call destroy on entity components.
     */
    void destroy() {
        for (auto i : m_entities) {
            i->destroy();
        }
    }

    /**
     * @brief Set the active camera for the scene.
     * @param camera Pointer to the camera.
     */
    void set_active_camera(raylib::Camera3D* camera) {
        m_active_camera = camera;
    }

    /**
     * @brief Draw scene.
     */
    void draw() {
        update();

        ClearBackground(BLACK);

        draw3D();

        m_renderer.calculate_shadows();

        get_active_camera().BeginMode();

        m_renderer.render(get_active_camera());

        get_active_camera().EndMode();

        draw2D();

        DrawFPS(10, 10);
    }

    void draw(raylib::RenderTexture2D& texture) {
        update();

        ClearBackground(BLACK);

        draw3D();

        m_renderer.calculate_shadows();

        texture.BeginMode();

        get_active_camera().BeginMode();

        m_renderer.render(get_active_camera());

        get_active_camera().EndMode();

        texture.EndMode();

        draw2D();

        DrawFPS(10, 10);
    }
};
} // namespace SPRF

#endif //_SPRB_ECS_HPP_