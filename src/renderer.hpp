#ifndef _SPRF_RENDERER_HPP_
#define _SPRF_RENDERER_HPP_

#include "raylib-cpp.hpp"
#include "shaders.hpp"
#include "shadow_map_texture.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace SPRF {
/**
 * @brief Class representing a model to be rendered.
 *
 * This class manages a 3D model and its instances, allowing for rendering with
 * different shaders.
 */
class RenderModel {
  private:
    /** @brief Shared pointer to the model */
    std::shared_ptr<raylib::Model> m_model;
    /** @brief Transformation matrix of the model */
    raylib::Matrix m_model_transform;
    /** @brief Instances of the model */
    std::vector<raylib::Matrix> m_instances;

  public:
    /**
     * @brief Construct a new RenderModel object.
     *
     * @param model Shared pointer to the model.
     */
    RenderModel(std::shared_ptr<raylib::Model> model)
        : m_model(model),
          m_model_transform(raylib::Matrix(m_model->GetTransform())) {}

    /**
     * @brief Clear all instances of the model.
     */
    void clear_instances() { m_instances.clear(); }

    /**
     * @brief Add an instance of the model.
     *
     * @param instance Transformation matrix of the instance.
     */
    void add_instance(raylib::Matrix instance) {
        m_instances.push_back(instance);
    }

    /**
     * @brief Draw the model with a specified shader.
     *
     * This method temporarily sets the shader of the model's materials to the
     * provided shader, draws all instances of the model, and then restores the
     * original shader.
     *
     * @param shader Shader to be used for drawing.
     */
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

    /**
     * @brief Draw the model with its default shader.
     */
    void draw() { draw(m_model->materials[0].shader); }
};

/**
 * @brief Class responsible for rendering models and managing lights and
 * shadows.
 *
 * This class manages a collection of render models and lights, allowing for the
 * rendering of 3D scenes with shadow mapping.
 */
class Renderer {
  private:
    /** @brief List of render models */
    std::vector<std::shared_ptr<RenderModel>> m_render_models;
    /** @brief Shader used for rendering */
    raylib::Shader m_shader;
    /** @brief Camera position uniform */
    ShaderUniform<raylib::Vector3> m_camera_position;
    /** @brief Ambient light coefficient uniform */
    ShaderUniform<float> m_ka;
    /** @brief List of lights */
    std::vector<std::shared_ptr<Light>> m_lights;
    /** @brief Shadow map resolution uniform */
    ShaderUniform<int> m_shadow_map_res;

  public:
    /**
     * @brief Construct a new Renderer object.
     *
     * @param ka Ambient light coefficient.
     * @param shadow_scale Shadow map resolution.
     */
    Renderer(float ka = 0.2, int shadow_scale = 1024)
        : m_shader(
              raylib::Shader("/Users/humzaqureshi/GitHub/"
                             "Super-Powered-Robot-Football/src/lights.vs",
                             "/Users/humzaqureshi/GitHub/"
                             "Super-Powered-Robot-Football/src/lights.fs")),
          m_ka("ka", ka, m_shader),
          m_camera_position("camPos", raylib::Vector3(0, 0, 0), m_shader),
          m_shadow_map_res("shadowMapRes", shadow_scale, m_shader) {}

    /**
     * @brief Create a render model and add it to the renderer.
     *
     * @param model Shared pointer to the model.
     * @return Shared pointer to the created render model.
     */
    std::shared_ptr<RenderModel>
    create_render_model(std::shared_ptr<raylib::Model> model) {
        m_render_models.push_back(std::make_shared<RenderModel>(model));
        return m_render_models[m_render_models.size() - 1];
    }

    /**
     * @brief Get the shader used by the renderer.
     *
     * @return Reference to the shader.
     */
    raylib::Shader& shader() { return m_shader; }

    /**
     * @brief Add a light to the renderer.
     *
     * This method creates a new light, adds it to the list of lights, and
     * returns it.
     *
     * @return Shared pointer to the created light.
     */
    std::shared_ptr<Light> add_light() {
        std::shared_ptr<Light> out =
            std::make_shared<Light>(m_shader, m_shadow_map_res.value());
        m_lights.push_back(out);
        return out;
    }

    /**
     * @brief Calculate shadows for all lights.
     *
     * This method iterates over all lights, and for each enabled light, it sets
     * up shadow mapping by rendering the scene from the light's perspective to
     * a shadow map.
     */
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

    /**
     * @brief Render the scene from the perspective of a camera.
     *
     * This method sets the camera position uniform, begins the camera mode,
     * renders all models, clears their instances, and ends the camera mode.
     *
     * @param camera Camera used to render the scene.
     */
    void render(raylib::Camera& camera) {
        m_camera_position.value(camera.GetPosition());
        camera.BeginMode();
        for (auto& i : m_render_models) {
            i->draw(m_shader);
            i->clear_instances();
        }
        camera.EndMode();
    }

    /**
     * @brief Set a new ambient light coefficient.
     *
     * @param v New ambient light coefficient.
     * @return Updated ambient light coefficient.
     */
    float ka(float v) { return m_ka.value(v); }

    /**
     * @brief Get the ambient light coefficient.
     *
     * @return Ambient light coefficient.
     */
    float ka() const { return m_ka.value(); }
};

} // namespace SPRF

#endif // _SPRF_RENDERER_HPP_