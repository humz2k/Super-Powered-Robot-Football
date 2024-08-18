#ifndef _SPRB_ECS_HPP_
#define _SPRB_ECS_HPP_

#include "base.hpp"
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
 *
 * Components are attached to entities and provide functionality such as
 * rendering, updating, and more.
 */
class Component : public Logger {
  private:
    /** @brief Pointer to the parent entity */
    Entity* m_entity = NULL;

  public:
    Component() {}
    /**
     * @brief Get the parent entity of the component.
     * @return Pointer to the parent entity.
     */
    Entity* entity() { return m_entity; }
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
     * @brief Update the component before the main update.
     */
    virtual void before_update() {}
    /**
     * @brief Update the component after the main update.
     */
    virtual void after_update() {}
    /**
     * @brief Draw3D.
     * @param transform Transformation matrix for drawing.
     */
    virtual void draw3D(raylib::Matrix transform) {}
    virtual void draw_debug() {}
    /**
     * @brief Draw2D.
     */
    virtual void draw2D() {}
};

/**
 * @brief Class representing a transform with position, rotation, and scale.
 */
class Transform : public Logger {
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
        // auto mat_scale = raylib::Matrix::Scale(scale.x, scale.y, scale.z);
        auto mat_rotation = raylib::Matrix::Rotate(rotationAxis, rotationAngle);
        auto mat_translation =
            raylib::Matrix::Translate(position.x, position.y, position.z);
        return mat_rotation * mat_translation; // * mat_scale;
    }

    raylib::Matrix rotation_matrix() {
        auto [rotationAxis, rotationAngle] =
            raylib::Quaternion::FromEuler(rotation).ToAxisAngle();
        return raylib::Matrix::Rotate(rotationAxis, rotationAngle);
    }
};

extern int id_counter;

/**
 * @brief Class representing an entity in the scene.
 *
 * Entities are containers for components and manage their lifecycle.
 */
class Entity : public Logger {
  private:
    /** @brief Components attached to the entity */
    std::unordered_map<std::type_index, Component*> m_components;
    /** @brief Children entities */
    std::vector<Entity*> m_children;
    /** @brief Transform of the entity */
    Transform m_transform;
    /** @brief Pointer to the scene */
    Scene* m_scene;
    /** @brief Pointer to the parent entity */
    Entity* m_parent = NULL;
    int m_id;

    /**
     * @brief Add a child entity.
     * @param child Pointer to the child entity.
     */
    void add_child(Entity* child) { m_children.push_back(child); }

    /**
     * @brief Call draw3D on components.
     * @param parent_transform Transformation matrix of the parent.
     */
    void draw3D(raylib::Matrix parent_transform) {
        raylib::Matrix transform = m_transform.matrix() * parent_transform;
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
     * @param scene Pointer to the scene.
     */
    Entity(Scene* scene) : m_scene(scene) {
        m_id = id_counter++;
        TraceLog(LOG_INFO, "created entity %d", m_id);
    };

    /**
     * @brief Construct a new Entity object with a parent entity.
     * @param scene Pointer to the scene.
     * @param parent Pointer to the parent entity.
     */
    Entity(Scene* scene, Entity* parent) : m_scene(scene), m_parent(parent) {
        m_id = id_counter++;
        TraceLog(LOG_INFO, "created entity %d", m_id);
    };

    /**
     * @brief Destroy the Entity object and its components.
     */
    ~Entity() {
        TraceLog(LOG_INFO, "deleting entity %d", m_id);
        for (const auto& [key, value] : m_components) {
            delete value;
        }
        for (auto i : m_children) {
            delete i;
        }
    }

    size_t n_children() { return m_children.size(); }

    Entity* get_child(int idx) {
        assert(idx >= 0);
        assert(idx < m_children.size());
        return m_children[idx];
    }

    /**
     * @brief Get the global transformation matrix.
     * @return Global transformation matrix.
     */
    raylib::Matrix global_transform() {
        if (!m_parent) {
            return m_transform.matrix();
        }
        return m_transform.matrix() * m_parent->global_transform();
    }

    raylib::Matrix global_rotation() {
        if (!m_parent) {
            return m_transform.rotation_matrix();
        }
        return m_transform.rotation_matrix() * m_parent->global_rotation();
    }

    /**
     * @brief Create a child entity.
     * @return Pointer to the child entity.
     */
    Entity* create_child() {
        Entity* out = new Entity(m_scene, this);
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
     * @brief Call draw_debug on components.
     */
    void draw_debug() {
        for (const auto& [key, value] : m_components) {
            value->draw_debug();
        }
        for (auto i : m_children) {
            i->draw_debug();
        }
    }

    /**
     * @brief Get a component of the entity.
     * @tparam T Type of the component.
     * @return Pointer to the component.
     */
    template <class T> T* get_component() {
        auto temp = m_components.find(std::type_index(typeid(T)));
        assert((temp != m_components.end()));
        return dynamic_cast<T*>(temp->second);
    }

    /**
     * @brief Add a component to the entity.
     * @tparam T Type of the component.
     * @tparam Args Parameter pack for the component constructor.
     * @param args Arguments for the component constructor.
     * @return Pointer to the added component.
     */
    template <class T, typename... Args> T* add_component(Args... args) {
        auto temp = m_components.find(std::type_index(typeid(T)));
        assert((temp == m_components.end()));
        m_components[std::type_index(typeid(T))] = new T(args...);
        m_components[std::type_index(typeid(T))]->set_parent(this);
        return get_component<T>();
    }

    /**
     * @brief Get the scene the entity belongs to.
     * @return Pointer to the scene.
     */
    Scene* scene() { return m_scene; }
};

/**
 * @brief Specialization for getting the Transform component.
 * @return Pointer to the Transform component.
 */
template <> Transform* Entity::get_component<Transform>();

/**
 * @brief Class representing a scene containing entities.
 *
 * A scene manages a collection of entities and coordinates their updates and
 * rendering.
 */
class Scene : public Logger {
  private:
    /** @brief Entities in the scene */
    std::vector<Entity*> m_entities;

    /** @brief Default camera in the scene */
    raylib::Camera3D m_default_camera;

    /** @brief Active camera in the scene */
    raylib::Camera3D* m_active_camera = NULL;

    /** @brief Renderer for the scene */
    Renderer m_renderer;

    raylib::Color m_background_color = raylib::Color::White();

    bool m_should_close = false;

    /**
     * @brief Add an entity to the scene.
     * @param entity Pointer to the entity.
     */
    void add_entity(Entity* entity) { m_entities.push_back(entity); }

    /**
     * @brief Update entity components.
     */
    void update() {
        size_t len = m_entities.size();
        for (size_t i = 0; i < len; i++) {
            m_entities[i]->update();
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
     * @brief Call draw3D on entity components.
     */
    void draw_debug() {
        for (auto i : m_entities) {
            i->draw_debug();
        }
    }

    /**
     * @brief Get the active camera of the scene.
     * @return Reference to the active camera.
     */
    raylib::Camera3D* get_active_camera() {
        if (!m_active_camera) {
            return &m_default_camera;
        }
        return m_active_camera;
    }

  public:
    template <typename... Args> Scene(Args... args) {}

    virtual ~Scene() {
        for (auto i : m_entities) {
            delete i;
        }
    }

    bool should_close() { return m_should_close; }

    void close() {
        TraceLog(LOG_INFO, "Closing scene...");
        m_should_close = true;
    }

    void set_background_color(raylib::Color color) {
        m_background_color = color;
    }

    /**
     * @brief Get the renderer for the scene.
     * @return Pointer to the renderer.
     */
    Renderer* renderer() { return &m_renderer; }

    /**
     * @brief Create a new entity in the scene.
     * @return Pointer to the created entity.
     */
    Entity* create_entity() {
        auto out = new Entity(this);
        add_entity(out);
        return out;
    }

    /**
     * @brief Initialize entity components.
     */
    void init() {
        size_t len = m_entities.size();
        for (size_t i = 0; i < len; i++) {
            m_entities[i]->init();
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
     * @brief Draw the entire scene to a render texture.
     *
     * This method updates all entities, clears the background, calculates
     * shadows, sets the active render texture, and renders all entities in 3D
     * and 2D.
     *
     * @param texture Render texture to draw to.
     */
    void draw(raylib::RenderTexture2D& texture) {
        update();

        ClearBackground(BLACK);

        draw3D();

        m_renderer.calculate_shadows(get_active_camera());

        texture.BeginMode();

        get_active_camera()->BeginMode();

        m_renderer.render(get_active_camera(), m_background_color);

        draw_debug();

        get_active_camera()->EndMode();

        texture.EndMode();
    }

    virtual void on_close() {}

    /**
     * @brief Call draw2D on entity components.
     */
    void draw2D() {
        for (auto i : m_entities) {
            i->draw2D();
        }
    }
};
} // namespace SPRF

#endif //_SPRB_ECS_HPP_