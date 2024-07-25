#ifndef _SPRF_RENDERER_HPP_
#define _SPRF_RENDERER_HPP_

#include "raylib-cpp.hpp"
#include "shaders.hpp"
#include "shadow_map_texture.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace SPRF {

class RenderModel {
  private:
    std::shared_ptr<raylib::Model> m_model;
    raylib::Matrix m_model_transform;
    std::vector<raylib::Matrix> m_instances;

  public:
    RenderModel(std::shared_ptr<raylib::Model> model)
        : m_model(model),
          m_model_transform(raylib::Matrix(m_model->GetTransform())) {}

    void clear_instances() { m_instances.clear(); }

    void add_instance(raylib::Matrix instance) {
        m_instances.push_back(instance);
    }

    void draw(Shader shader) {
        Shader old_shader = m_model->materials[0].shader;
        m_model->materials[0].shader = shader;
        for (auto& transform : m_instances) {
            raylib::Matrix final_transform = m_model_transform * transform;
            for (int i = 0; i < m_model->meshCount; i++) {
                DrawMesh(m_model->meshes[i],
                         m_model->materials[m_model->meshMaterial[i]],
                         final_transform);
            }
        }
        m_model->materials[0].shader = old_shader;
    }

    void draw() { draw(m_model->materials[0].shader); }
};

class Renderer {
  private:
    std::vector<std::shared_ptr<RenderModel>> m_render_models;
    raylib::Shader m_shader;
    ShaderUniform<raylib::Vector3> m_camera_position;
    ShaderUniform<float> m_ka;
    std::vector<std::shared_ptr<Light>> m_lights;
    ShaderUniform<int> m_shadow_map_res;

  public:
    Renderer(float ka = 0.2, int shadow_scale = 1024)
        : m_shader(
              raylib::Shader("/Users/humzaqureshi/GitHub/"
                             "Super-Powered-Robot-Football/src/lights.vs",
                             "/Users/humzaqureshi/GitHub/"
                             "Super-Powered-Robot-Football/src/lights.fs")),
          m_ka("ka", ka, m_shader),
          m_camera_position("camPos", raylib::Vector3(0, 0, 0), m_shader),
          m_shadow_map_res("shadowMapRes", shadow_scale, m_shader) {}

    std::shared_ptr<RenderModel>
    create_render_model(std::shared_ptr<raylib::Model> model) {
        m_render_models.push_back(std::make_shared<RenderModel>(model));
        return m_render_models[m_render_models.size() - 1];
    }

    raylib::Shader& shader() { return m_shader; }

    std::shared_ptr<Light> add_light() {
        std::shared_ptr<Light> out =
            std::make_shared<Light>(m_shader, m_shadow_map_res.value());
        m_lights.push_back(out);
        return out;
    }

    void calculate_shadows() {
        int slot_start = 15 - MAX_LIGHTS;
        assert(m_lights.size() <= MAX_LIGHTS);
        for (auto& light : m_lights) {
            if (!light->enabled())
                continue;
            raylib::Camera3D light_cam = light->light_cam();
            light->BeginShadowMode();
            ClearBackground(BLACK);

            for (auto& i : m_render_models) {
                i->draw();
            }
            light->EndShadowMode(slot_start);
        }
    }

    void render(raylib::Camera& camera) {
        m_camera_position.value(camera.GetPosition());
        camera.BeginMode();
        for (auto& i : m_render_models) {
            i->draw(m_shader);
            i->clear_instances();
        }
        camera.EndMode();
    }

    float ka(float v) { return m_ka.value(v); }

    float ka() const { return m_ka.value(); }
};

} // namespace SPRF

#endif // _SPRF_RENDERER_HPP_