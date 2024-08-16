#ifndef _SPRF_NETWORKING_MAP_HPP_
#define _SPRF_NETWORKING_MAP_HPP_

#include "custom_mesh.hpp"
#include "engine/engine.hpp"
#include "raylib-cpp.hpp"
#include <ode/ode.h>
#include <string>
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

class MapPlaneElement : public MapElement {
  private:
    float m_x_size;
    float m_y_size;
    std::string m_texture_path;
    int m_resX;
    int m_resY;

  public:
    MapPlaneElement(float x_size, float y_size, std::string texture_path = "",
                    int resX = 10, int resY = 10)
        : m_x_size(x_size), m_y_size(y_size), m_texture_path(texture_path),
          m_resX(resX), m_resY(resY) {}

    void load(Scene* scene) {
        auto plane = scene->renderer()->create_render_model(
            WrappedMesh(m_x_size, m_y_size, m_resX, m_resY));
        plane->clip(false);
        if (m_texture_path != "")
            plane->add_texture(m_texture_path);

        for (auto& i : this->instances()) {
            auto entity = scene->create_entity();
            entity->add_component<Model>(plane);
            entity->get_component<Transform>()->position = i.position;
            entity->get_component<Transform>()->rotation = i.rotation;
        }
    }

    void load(dWorldID world, dSpaceID space) {}
};

class MapLightElement : public MapElement {
  private:
    raylib::Vector3 m_L;
    raylib::Vector3 m_target;
    float m_fov;

  public:
    MapLightElement(raylib::Vector3 L,
                    raylib::Vector3 target = raylib::Vector3(0, 0, 0),
                    float fov = 70)
        : m_L(L), m_target(target), m_fov(fov) {}

    void load(Scene* scene) {
        auto light = scene->renderer()->add_light();
        light->L(m_L);
        light->target(m_target);
        light->fov(m_fov);
        light->enabled(1);
    }

    void load(dWorldID world, dSpaceID space) {}
};

class MapSkyboxElement : public MapElement {
  private:
    std::string m_path;

  public:
    MapSkyboxElement(std::string path) : m_path(path) {}
    void load(Scene* scene) {
        scene->renderer()->load_skybox(m_path);
        scene->renderer()->enable_skybox();
    }
    void load(dWorldID world, dSpaceID space) {}
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

    out->add_element(std::make_shared<MapLightElement>(
        raylib::Vector3(1, 2, 0.02), raylib::Vector3(2.5, 0, 0), 70));

    out->add_element(
        std::make_shared<MapSkyboxElement>("src/defaultskybox.png"));

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

    std::shared_ptr<MapPlaneElement> ground_plane =
        std::make_shared<MapPlaneElement>(70, 60,
                                          "assets/prototype_texture/grey4.png");
    ground_plane->add_instance(raylib::Vector3(0, 0, 0),
                               raylib::Vector3(0, 0, 0));
    out->add_element(ground_plane);

    std::shared_ptr<MapPlaneElement> wall_plane =
        std::make_shared<MapPlaneElement>(
            10, 10, "assets/prototype_texture/orange.png");
    wall_plane->add_instance(raylib::Vector3(0, 5, -30),
                             raylib::Vector3(M_PI_2, 0, 0));
    out->add_element(wall_plane);
    return out;
}

} // namespace SPRF

#endif // _SPRF_NETWORKING_MAP_HPP_