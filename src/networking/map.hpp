#ifndef _SPRF_NETWORKING_MAP_HPP_
#define _SPRF_NETWORKING_MAP_HPP_

#include "custom_mesh.hpp"
#include "editor/editor_tools.hpp"
#include "engine/engine.hpp"
#include "raylib-cpp.hpp"
#include <ode/ode.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace SPRF {

struct MapElementInstance {
    raylib::Vector3 position;
    raylib::Vector3 rotation;
    MapElementInstance() {}
    MapElementInstance(raylib::Vector3 position_, raylib::Vector3 rotation_)
        : position(position_), rotation(rotation_) {}
    json serialize(){
        json out;
        out["pos"] = {position.x,position.y,position.z};
        out["rot"] = {rotation.x,rotation.y,rotation.z};
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

    static void reset_ids(){
        m_next_id = 0;
    }

    int fresh_id(){
        int out = m_next_id;
        m_next_id++;
        return out;
    }

    std::vector<MapElementInstance>& instances() { return m_instances; }

    void add_instance(raylib::Vector3 position, raylib::Vector3 rotation) {
        m_instances.push_back(MapElementInstance(position, rotation));
    }

    void read_instances(json j){
        for (auto& i : j){
            raylib::Vector3 pos(i["pos"][0],i["pos"][1],i["pos"][2]);
            raylib::Vector3 rot(i["rot"][0],i["rot"][1],i["rot"][2]);
            add_instance(pos,rot);
        }
    }

    //void load_instances(json j){

    //}

    virtual void load(Scene* scene) = 0;

    virtual void load(dWorldID world, dSpaceID space) = 0;

    virtual void load_editor(Scene* scene) { this->load(scene); }

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

    void load(Scene* scene) {
        auto model = scene->renderer()->create_render_model(
            raylib::Mesh::Cube(m_width, m_height, m_length));
        if (m_texture_path != "")
            model->add_texture(m_texture_path);
        auto map_entity = scene->find_entity("sprf_map");
        assert(map_entity);
        auto parent = map_entity->create_child("map_cube_element_" + std::to_string(fresh_id()));
        for (auto& i : this->instances()) {
            auto entity = parent->create_child();
            entity->add_component<Model>(model);
            entity->get_component<Transform>()->position = i.position;
            entity->get_component<Transform>()->rotation = i.rotation;
        }
    }

    void load_editor(Scene* scene) {
        auto model = scene->renderer()->create_render_model(
            raylib::Mesh::Cube(m_width, m_height, m_length));
        if (m_texture_path != "")
            model->add_texture(m_texture_path);
        auto map_entity = scene->find_entity("sprf_map");
        assert(map_entity);
        auto parent = map_entity->create_child("map_cube_element_" + std::to_string(fresh_id()));
        for (auto& i : this->instances()) {
            auto entity = parent->create_child("cube");
            entity->add_component<Model>(model);
            entity->get_component<Transform>()->position = i.position;
            entity->get_component<Transform>()->rotation = i.rotation;
            entity->add_component<Selectable>(true, true);
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

    json serialize(){
        json out;
        out["type"] = "MapCubeElement";
        json params;
        params["size"] = {m_width,m_height,m_length};
        params["texture"] = m_texture_path;
        std::vector<json> instances;
        for (auto& i : this->instances()){
            instances.push_back(i.serialize());
        }
        params["instances"] = instances;
        out["params"] = params;
        return out;
    }

    MapCubeElement(json params){
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

    void load(Scene* scene) {
        auto plane = scene->renderer()->create_render_model(
            WrappedMesh(m_x_size, m_y_size, m_resX, m_resY));
        plane->clip(false);
        if (m_texture_path != "")
            plane->add_texture(m_texture_path);
        auto map_entity = scene->find_entity("sprf_map");
        assert(map_entity);
        auto parent = map_entity->create_child("map_plane_element" + std::to_string(fresh_id()));
        for (auto& i : this->instances()) {
            auto entity = parent->create_child();
            entity->add_component<Model>(plane);
            entity->get_component<Transform>()->position = i.position;
            entity->get_component<Transform>()->rotation = i.rotation;
        }
    }

    void load_editor(Scene* scene) {
        auto plane = scene->renderer()->create_render_model(
            WrappedMesh(m_x_size, m_y_size, m_resX, m_resY));
        plane->clip(false);
        if (m_texture_path != "")
            plane->add_texture(m_texture_path);
        auto map_entity = scene->find_entity("sprf_map");
        assert(map_entity);
        auto parent = map_entity->create_child("map_plane_element_" + std::to_string(fresh_id()));
        for (auto& i : this->instances()) {
            auto entity = parent->create_child("plane");
            entity->add_component<Model>(plane);
            entity->get_component<Transform>()->position = i.position;
            entity->get_component<Transform>()->rotation = i.rotation;
            entity->add_component<Selectable>(true, true);
        }
    }

    void load(dWorldID world, dSpaceID space) {}

    json serialize(){
        json out;
        out["type"] = "MapPlaneElement";
        json params;
        params["size"] = {m_x_size,m_y_size};
        params["res"] = {m_resX,m_resY};
        params["texture"] = m_texture_path;
        std::vector<json> instances;
        for (auto& i : this->instances()){
            instances.push_back(i.serialize());
        }
        params["instances"] = instances;
        out["params"] = params;
        return out;
    }

    MapPlaneElement(json params){
        read_instances(params["instances"]);
        m_x_size = params["size"][0];
        m_y_size = params["size"][1];
        m_resX = params["res"][0];
        m_resY = params["res"][1];
        m_texture_path = params["texture"];
    }
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

    MapLightElement(json params){
        m_L.x = params["L"][0];
        m_L.y = params["L"][1];
        m_L.z = params["L"][2];
        m_target.x = params["target"][0];
        m_target.y = params["target"][1];
        m_target.z = params["target"][2];
        m_fov = params["fov"];
    }

    void load(Scene* scene) {
        auto light = scene->renderer()->add_light();
        light->L(m_L);
        light->target(m_target);
        light->fov(m_fov);
        light->enabled(1);
    }

    void load(dWorldID world, dSpaceID space) {}

    json serialize(){
        json out;
        out["type"] = "MapLightElement";
        json params;
        params["L"] = {m_L.x,m_L.y,m_L.z};
        params["target"] = {m_target.x,m_target.y,m_target.z};
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
    void load(Scene* scene) {
        scene->renderer()->load_skybox(m_path);
        scene->renderer()->enable_skybox();
    }
    void load(dWorldID world, dSpaceID space) {}

    json serialize(){
        json out;
        out["type"] = "MapSkyboxElement";
        json params;
        params["path"] = m_path;
        out["params"] = params;
        return out;
    }

    //MapSkyboxElement(json params){
    //    m_path = params["path"];
    //}
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
        MapElement::reset_ids();
        scene->create_entity("sprf_map");
        for (auto i : m_elements) {
            i->load(scene);
        }
    }

    void load_editor(Scene* scene) {
        MapElement::reset_ids();
        scene->create_entity("sprf_map");
        for (auto i : m_elements) {
            i->load_editor(scene);
        }
    }

    void load(dWorldID world, dSpaceID space) {
        for (auto i : m_elements) {
            i->load(world, space);
        }
    }

    void save(std::string filename){
        json j;
        j["filename"] = filename;
        std::vector<json> elements;
        for (auto& i : m_elements){
            elements.push_back(i->serialize());
        }
        j["elements"] = elements;
        std::ofstream o(filename);
        o << std::setw(2) << j << std::endl;
    }

    void read(std::string filename){
        std::ifstream f(filename);
        json data = json::parse(f);
        TraceLog(LOG_INFO,"opening map %s",((std::string)data["filename"]).c_str());
        std::vector<json> elements = data["elements"];
        for (auto& i : elements){
            TraceLog(LOG_INFO,"reading element type %s",std::string(i["type"]).c_str());
            if (i["type"] == "MapLightElement"){
                add_element(std::make_shared<MapLightElement>(i["params"]));
            } else if (i["type"] == "MapSkyboxElement"){
                add_element(std::make_shared<MapSkyboxElement>(i["params"]["path"]));
            } else if (i["type"] == "MapPlaneElement"){
                add_element(std::make_shared<MapPlaneElement>(i["params"]));
            } else if (i["type"] == "MapCubeElement"){
                add_element(std::make_shared<MapCubeElement>(i["params"]));
            } else {
                TraceLog(LOG_ERROR,"unknown element type %s",std::string(i["type"]).c_str());
            }
        }
    }
};

static void load_map(std::string name, Scene* scene){
    Map map;
    map.read(name);
    map.load(scene);
}

static void load_map(std::string name, dWorldID world, dSpaceID space){
    Map map;
    map.read(name);
    map.load(world,space);
}

static std::shared_ptr<Map> simple_map() {
    std::shared_ptr<Map> out = std::make_shared<Map>();

    out->add_element(std::make_shared<MapLightElement>(
        raylib::Vector3(1, 2, 0.02), raylib::Vector3(2.5, 0, 0), 70));

    out->add_element(
        std::make_shared<MapSkyboxElement>("assets/defaultskybox.png"));

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