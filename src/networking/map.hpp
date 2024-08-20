/** @file map.hpp
 * @brief mapping stuff
 *
 * To add a map element: implement MapElement, and add the element to Map::Read.
 *
 */


#ifndef _SPRF_NETWORKING_MAP_HPP_
#define _SPRF_NETWORKING_MAP_HPP_

#include "custom_mesh.hpp"
#include "editor/editor_tools.hpp"
#include "engine/engine.hpp"
#include "raylib-cpp.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <ode/ode.h>
#include <string>
#include <vector>
using json = nlohmann::json;

namespace SPRF {

/** @struct MapElementInstance
 * @brief this is serialized as a list of {"pos": {x,y,z}, "rot": {x,y,z}, "scale": {x,y,z}}
 */
struct MapElementInstance {
    vec3 position;
    vec3 rotation;
    vec3 scale;
    MapElementInstance() {}
    MapElementInstance(vec3 position_, vec3 rotation_, vec3 scale_ = vec3(1,1,1))
        : position(position_), rotation(rotation_), scale(scale_) {}
    json serialize() {
        json out;
        out["pos"] = {position.x, position.y, position.z};
        out["rot"] = {rotation.x, rotation.y, rotation.z};
        out["scale"] = {scale.x, scale.y, scale.z};
        return out;
    }
};

class MapElement {
  private:
    static int m_next_id;
    std::vector<MapElementInstance> m_instances;

  public:
    MapElement() {}

    virtual ~MapElement() {}

    static void reset_ids() { m_next_id = 0; }

    int fresh_id() {
        int out = m_next_id;
        m_next_id++;
        return out;
    }

    std::vector<MapElementInstance>& instances() { return m_instances; }

    void add_instance(vec3 position, vec3 rotation, vec3 scale = vec3(1,1,1)) {
        m_instances.push_back(MapElementInstance(position, rotation, scale));
    }

    void read_instances(json j) {
        for (auto& i : j) {
            vec3 pos(i["pos"][0], i["pos"][1], i["pos"][2]);
            vec3 rot(i["rot"][0], i["rot"][1], i["rot"][2]);
            vec3 scale(i["scale"][0], i["scale"][1], i["scale"][2]);
            add_instance(pos, rot, scale);
        }
    }

    // load into the scene. try to keep everything organized -> make child of sprf_map etc..
    // editor flag is set when loading into the editor instead of game.
    virtual void load(Scene* scene, bool editor) = 0;

    // load into physics world -> without needing to specify positions for any reason
    virtual void load(dWorldID world, dSpaceID space) = 0;

    // load into physics world -> with specifying positions
    virtual void
    load(dWorldID world, dSpaceID space,
         std::unordered_map<std::string, std::vector<MapElementInstance>>&
             positions) {
        this->load(world, space);
    }

    virtual json serialize() = 0;
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

    void load(Scene* scene, bool editor) {
        auto model = scene->renderer()->create_render_model(
            Mesh::Cube(m_width, m_height, m_length));
        if (m_texture_path != "")
            model->add_texture(m_texture_path);
        auto map_entity = scene->find_entity("sprf_map");
        assert(map_entity);
        auto parent = map_entity->create_child("map_cube_element_" +
                                               std::to_string(fresh_id()));
        for (auto& i : this->instances()) {
            auto entity = parent->create_child();
            entity->add_component<Model>(model);
            entity->get_component<Transform>()->position = i.position;
            entity->get_component<Transform>()->rotation = i.rotation;
            if (editor){
                entity->add_component<Selectable>(true, true);
            }
        }
    }

    void load(dWorldID world, dSpaceID space) {
        for (auto& i : this->instances()) {
            auto geom = dCreateBox(space, m_width, m_height, m_length);
            dGeomSetPosition(geom, i.position.x, i.position.y, i.position.z);
            dMatrix3 rotation;
            auto mat = mat4x4::RotateXYZ(i.rotation);
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

    json serialize() {
        json out;
        out["type"] = "MapCubeElement";
        json params;
        params["size"] = {m_width, m_height, m_length};
        params["texture"] = m_texture_path;
        std::vector<json> instances;
        for (auto& i : this->instances()) {
            instances.push_back(i.serialize());
        }
        params["instances"] = instances;
        out["params"] = params;
        return out;
    }

    MapCubeElement(json params) {
        read_instances(params["instances"]);
        m_width = params["size"][0];
        m_height = params["size"][1];
        m_length = params["size"][2];
        m_texture_path = params["texture"];
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

    void load(Scene* scene, bool editor) {
        auto plane = scene->renderer()->create_render_model(
            WrappedMesh(m_x_size, m_y_size, m_resX, m_resY));
        plane->clip(false);
        if (m_texture_path != "")
            plane->add_texture(m_texture_path);
        auto map_entity = scene->find_entity("sprf_map");
        assert(map_entity);
        auto parent = map_entity->create_child("map_plane_element" +
                                               std::to_string(fresh_id()));
        for (auto& i : this->instances()) {
            auto entity = parent->create_child();
            entity->add_component<Model>(plane);
            entity->get_component<Transform>()->position = i.position;
            entity->get_component<Transform>()->rotation = i.rotation;
            if (editor){
                entity->add_component<Selectable>(true, true);
            }
        }
    }

    void load(dWorldID world, dSpaceID space) {}

    json serialize() {
        json out;
        out["type"] = "MapPlaneElement";
        json params;
        params["size"] = {m_x_size, m_y_size};
        params["res"] = {m_resX, m_resY};
        params["texture"] = m_texture_path;
        std::vector<json> instances;
        for (auto& i : this->instances()) {
            instances.push_back(i.serialize());
        }
        params["instances"] = instances;
        out["params"] = params;
        return out;
    }

    MapPlaneElement(json params) {
        read_instances(params["instances"]);
        m_x_size = params["size"][0];
        m_y_size = params["size"][1];
        m_resX = params["res"][0];
        m_resY = params["res"][1];
        m_texture_path = params["texture"];
    }
};

class MapPositionElement : public MapElement {
  private:
    std::string m_name;

  public:
    MapPositionElement(std::string name) : m_name(name) {}

    void load(dWorldID world, dSpaceID space,
              std::unordered_map<std::string, std::vector<MapElementInstance>>&
                  positions) {
        positions[m_name] = instances();
    }

    void load(dWorldID world, dSpaceID space) {}

    void load(Scene* scene, bool editor) {
        auto map_entity = scene->find_entity("sprf_map");
        assert(map_entity);
        auto parent =
            map_entity->create_child("map_position_element_" + m_name + "_" +
                                     std::to_string(fresh_id()));
        for (auto& i : this->instances()) {
            auto entity = parent->create_child("position");
            entity->get_component<Transform>()->position = i.position;
            entity->get_component<Transform>()->rotation = i.rotation;
            if (editor){
                entity->add_component<Selectable>(true, true);
            }
        }
    }

    json serialize() {
        json out;
        out["type"] = "MapPositionElement";
        json params;
        params["name"] = m_name;
        std::vector<json> instances;
        for (auto& i : this->instances()) {
            instances.push_back(i.serialize());
        }
        params["instances"] = instances;
        out["params"] = params;
        return out;
    }
};

class MapLightElement : public MapElement {
  private:
    vec3 m_L;
    vec3 m_target;
    float m_fov;

  public:
    MapLightElement(vec3 L,
                    vec3 target = vec3(0, 0, 0),
                    float fov = 70)
        : m_L(L), m_target(target), m_fov(fov) {}

    MapLightElement(json params) {
        m_L.x = params["L"][0];
        m_L.y = params["L"][1];
        m_L.z = params["L"][2];
        m_target.x = params["target"][0];
        m_target.y = params["target"][1];
        m_target.z = params["target"][2];
        m_fov = params["fov"];
    }

    void load(Scene* scene, bool editor) {
        auto light = scene->renderer()->add_light();
        light->L(m_L);
        light->target(m_target);
        light->fov(m_fov);
        light->enabled(1);
    }

    void load(dWorldID world, dSpaceID space) {}

    json serialize() {
        json out;
        out["type"] = "MapLightElement";
        json params;
        params["L"] = {m_L.x, m_L.y, m_L.z};
        params["target"] = {m_target.x, m_target.y, m_target.z};
        params["fov"] = m_fov;
        out["params"] = params;
        return out;
    }
};

class MapSkyboxElement : public MapElement {
  private:
    std::string m_path;

  public:
    MapSkyboxElement(std::string path) : m_path(path) {}
    void load(Scene* scene, bool editor) {
        scene->renderer()->load_skybox(m_path);
        scene->renderer()->enable_skybox();
    }
    void load(dWorldID world, dSpaceID space) {}

    json serialize() {
        json out;
        out["type"] = "MapSkyboxElement";
        json params;
        params["path"] = m_path;
        out["params"] = params;
        return out;
    }
};

class Map {
  private:
    std::vector<std::shared_ptr<MapElement>> m_elements;

  public:
    Map() {}

    Map(std::string filename) { read(filename); }

    void add_element(std::shared_ptr<MapElement> element) {
        m_elements.push_back(element);
    }

    void load(Scene* scene) {
        MapElement::reset_ids();
        scene->create_entity("sprf_map");
        for (auto i : m_elements) {
            i->load(scene,false);
        }
    }

    void load_editor(Scene* scene) {
        MapElement::reset_ids();
        scene->create_entity("sprf_map");
        for (auto i : m_elements) {
            i->load(scene,true);
        }
    }

    void load(dWorldID world, dSpaceID space,
              std::unordered_map<std::string, std::vector<MapElementInstance>>&
                  positions) {
        for (auto i : m_elements) {
            i->load(world, space, positions);
        }
    }

    void save(std::string filename) {
        json j;
        j["filename"] = filename;
        std::vector<json> elements;
        for (auto& i : m_elements) {
            elements.push_back(i->serialize());
        }
        j["elements"] = elements;
        std::ofstream o(filename);
        o << std::setw(2) << j << std::endl;
    }

    void read(std::string filename) {
        std::ifstream f(filename);
        json data = json::parse(f);
        TraceLog(LOG_INFO, "opening map %s",
                 ((std::string)data["filename"]).c_str());
        std::vector<json> elements = data["elements"];
        for (auto& i : elements) {
            TraceLog(LOG_INFO, "reading element type %s",
                     std::string(i["type"]).c_str());
            if (i["type"] == "MapLightElement") {
                add_element(std::make_shared<MapLightElement>(i["params"]));
            } else if (i["type"] == "MapSkyboxElement") {
                add_element(
                    std::make_shared<MapSkyboxElement>(i["params"]["path"]));
            } else if (i["type"] == "MapPlaneElement") {
                add_element(std::make_shared<MapPlaneElement>(i["params"]));
            } else if (i["type"] == "MapCubeElement") {
                add_element(std::make_shared<MapCubeElement>(i["params"]));
            } else if (i["type"] == "MapPositionElement") {
                auto wtf = std::make_shared<MapPositionElement>(
                    std::string(i["params"]["name"]));
                wtf->read_instances(i["params"]["instances"]);
                add_element(wtf);
            } else {
                TraceLog(LOG_ERROR, "unknown element type %s",
                         std::string(i["type"]).c_str());
            }
        }
    }
};

static std::shared_ptr<Map> simple_map() {
    std::shared_ptr<Map> out = std::make_shared<Map>();

    std::shared_ptr<MapPositionElement> ball_start =
        std::make_shared<MapPositionElement>("ball_start");
    ball_start->add_instance(vec3(2, 2, 2),
                             vec3(0, 0, 0));
    out->add_element(ball_start);

    std::shared_ptr<MapPositionElement> team_1_spawns =
        std::make_shared<MapPositionElement>("team_1_spawns");
    team_1_spawns->add_instance(vec3(5, 2, 5),
                                vec3(0, 0, 0));
    team_1_spawns->add_instance(vec3(7, 2, 5),
                                vec3(0, 0, 0));
    out->add_element(team_1_spawns);

    std::shared_ptr<MapPositionElement> team_2_spawns =
        std::make_shared<MapPositionElement>("team_2_spawns");
    team_2_spawns->add_instance(vec3(5, 2, 10),
                                vec3(0, 0, 0));
    team_2_spawns->add_instance(vec3(7, 2, 10),
                                vec3(0, 0, 0));
    out->add_element(team_2_spawns);

    out->add_element(std::make_shared<MapLightElement>(
        vec3(1, 2, 0.02), vec3(2.5, 0, 0), 70));

    out->add_element(
        std::make_shared<MapSkyboxElement>("assets/defaultskybox.png"));

    std::shared_ptr<MapCubeElement> basic_block =
        std::make_shared<MapCubeElement>(0.5, 0.5, 0.5,
                                         "assets/prototype_texture/blue2.png");
    basic_block->add_instance(vec3(5, 0.25, 5),
                              vec3(0.2, 0.2, 0.2));
    basic_block->add_instance(vec3(3, 0.25, 3),
                              vec3(0, 0, 0));
    basic_block->add_instance(vec3(1, 0.25, 1),
                              vec3(0.5, 0, 0));
    out->add_element(basic_block);

    std::shared_ptr<MapPlaneElement> ground_plane =
        std::make_shared<MapPlaneElement>(70, 60,
                                          "assets/prototype_texture/grey4.png");
    ground_plane->add_instance(vec3(0, 0, 0),
                               vec3(0, 0, 0));
    out->add_element(ground_plane);

    std::shared_ptr<MapPlaneElement> wall_plane =
        std::make_shared<MapPlaneElement>(
            10, 10, "assets/prototype_texture/orange.png");
    wall_plane->add_instance(vec3(0, 5, -30),
                             vec3(M_PI_2, 0, 0));
    out->add_element(wall_plane);
    return out;
}

} // namespace SPRF

#endif // _SPRF_NETWORKING_MAP_HPP_