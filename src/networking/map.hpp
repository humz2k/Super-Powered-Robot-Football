#ifndef _SPRF_NETWORKING_MAP_HPP_
#define _SPRF_NETWORKING_MAP_HPP_

#include "engine/engine.hpp"
#include "raylib-cpp.hpp"
#include <ode/ode.h>
#include <vector>

namespace SPRF {

struct MapElementInstance {
    raylib::Vector3 position;
    raylib::Vector3 rotation;
    MapElementInstance() {}
    MapElementInstance(raylib::Vector3 position_, raylib::Vector3 rotation_)
        : position(position_), rotation(rotation_) {}
};

class MapElement {
  private:
    std::vector<MapElementInstance> m_instances;

  public:
    MapElement() {}

    virtual ~MapElement() {}

    std::vector<MapElementInstance>& instances() { return m_instances; }

    void add_instance(raylib::Vector3 position, raylib::Vector3 rotation) {
        m_instances.push_back(MapElementInstance(position, rotation));
    }

    virtual void load(Scene* scene) = 0;

    virtual void load(dWorldID world, dSpaceID space) = 0;
};

class MapCubeElement : public MapElement {
  private:
    float m_width;
    float m_height;
    float m_length;
    std::string m_texture_path;

  public:
    MapCubeElement(float width, float height, float length,
                   std::string texture_path = "")
        : m_width(width), m_height(height), m_length(length),
          m_texture_path(texture_path) {}

    void load(Scene* scene) {
        auto model = scene->renderer()->create_render_model(
            raylib::Mesh::Cube(m_width, m_height, m_length));
        if (m_texture_path != "")
            model->add_texture(m_texture_path);
        for (auto& i : this->instances()) {
            auto entity = scene->create_entity();
            entity->add_component<Model>(model);
            entity->get_component<Transform>()->position = i.position;
            entity->get_component<Transform>()->rotation = i.rotation;
        }
    }

    void load(dWorldID world, dSpaceID space) {
        for (auto& i : this->instances()) {
            auto geom = dCreateBox(space, m_width, m_height, m_length);
            dGeomSetPosition(geom, i.position.x, i.position.y, i.position.z);
            dMatrix3 rotation;
            auto mat = raylib::Matrix::RotateXYZ(i.rotation);
            // mat.
            // rotation[0] =
            // dRFromEulerAngles(rotation,i.rotation.x,i.rotation.y,i.rotation.z);
            rotation[0] = mat.m0;
            rotation[1] = mat.m4;
            rotation[2] = mat.m8;
            rotation[3] = mat.m12;
            rotation[4] = mat.m1;
            rotation[5] = mat.m5;
            rotation[6] = mat.m9;
            rotation[7] = mat.m13;
            rotation[8] = mat.m2;
            rotation[9] = mat.m6;
            rotation[10] = mat.m10;
            rotation[11] = mat.m14;
            dGeomSetRotation(geom, rotation);
        }
    }
};

class Map {
  private:
    std::vector<std::shared_ptr<MapElement>> m_elements;

  public:
    Map() {}

    void add_element(std::shared_ptr<MapElement> element) {
        m_elements.push_back(element);
    }

    void load(Scene* scene) {
        for (auto i : m_elements) {
            i->load(scene);
        }
    }

    void load(dWorldID world, dSpaceID space) {
        for (auto i : m_elements) {
            i->load(world, space);
        }
    }
};

static std::shared_ptr<Map> simple_map() {
    std::shared_ptr<Map> out = std::make_shared<Map>();
    std::shared_ptr<MapCubeElement> basic_block =
        std::make_shared<MapCubeElement>(0.5, 0.5, 0.5,
                                         "assets/prototype_texture/blue2.png");
    basic_block->add_instance(raylib::Vector3(5, 0.25, 5),
                              raylib::Vector3(0.2, 0.2, 0.2));
    basic_block->add_instance(raylib::Vector3(3, 0.25, 3),
                              raylib::Vector3(0, 0, 0));
    basic_block->add_instance(raylib::Vector3(1, 0.25, 1),
                              raylib::Vector3(0.5, 0, 0));
    out->add_element(basic_block);
    return out;
}

} // namespace SPRF

#endif // _SPRF_NETWORKING_MAP_HPP_