#ifndef _SPRF_RENDERER_HPP_
#define _SPRF_RENDERER_HPP_

#include "base.hpp"
#include "raylib-cpp.hpp"
#include "shaders.hpp"
#include "shadow_map_texture.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace SPRF {

struct BBox {
    raylib::Vector3 c1, c2, c3, c4, c5, c6, c7, c8;
    BBox(BoundingBox bbox) {
        c1 = bbox.min;
        c2 = bbox.min;
        c2.z = bbox.max.z;
        c3 = bbox.min;
        c3.x = bbox.max.x;
        c4 = bbox.min;
        c4.y = bbox.max.y;
        c5 = bbox.min;
        c5.x = bbox.max.x;
        c5.z = bbox.max.z;
        c6 = bbox.min;
        c6.y = bbox.max.y;
        c6.z = bbox.max.z;
        c7 = bbox.min;
        c7.y = bbox.max.y;
        c7.x = bbox.max.x;
        c8 = bbox.max;
    }

    BBox() {}

    bool visible(raylib::Matrix matrix) {
        Vector3* points = (Vector3*)this;
        for (int i = 0; i < 8; i++) {
            if (Vector3Transform(points[i], matrix).z <= 0)
                return true;
        }
        return false;
    }
};

/**
 * @brief Class representing a model to be rendered.
 *
 * This class manages a 3D model and its instances, allowing for rendering with
 * different shaders.
 */
class RenderModel : public Logger {
  private:
    /** @brief Shared pointer to the model */
    raylib::Model* m_model;
    BBox* m_bounding_boxes;
    // std::vector<
    /** @brief Transformation matrix of the model */
    raylib::Matrix m_model_transform;
    /** @brief Instances of the model */
    std::vector<raylib::Matrix> m_instances;

  public:
    template <typename... Args>
    RenderModel(Args... args)
        : m_model(new raylib::Model(args...)),
          m_model_transform(raylib::Matrix(m_model->GetTransform())) {
        m_bounding_boxes = (BBox*)malloc(sizeof(BBox));
        for (int i = 0; i < m_model->meshCount; i++) {
            m_bounding_boxes[i] =
                (BoundingBox(GetMeshBoundingBox(m_model->meshes[i])));
        }
    }

    ~RenderModel() {
        delete m_model;
        free(m_bounding_boxes);
    }

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
        m_instances.push_back(m_model_transform * instance);
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
    void draw(Shader shader, raylib::Matrix vp) {
        Shader old_shader = m_model->materials[0].shader;
        m_model->materials[0].shader = shader;
        for (int i = 0; i < m_model->meshCount; i++) {
            BBox bbox = m_bounding_boxes[i];
            Mesh mesh = m_model->meshes[i];
            Material material = m_model->materials[m_model->meshMaterial[i]];
            for (auto& transform : m_instances) {
                raylib::Matrix final_transform = transform;
                if (bbox.visible(final_transform * vp)) {
                    game_info.visible_meshes++;
                    DrawMesh(mesh, material, final_transform);
                }
            }
        }
        m_model->materials[0].shader = old_shader;
    }

    /**
     * @brief Draw the model with its default shader.
     */
    void draw(raylib::Matrix vp) { draw(m_model->materials[0].shader, vp); }
};

/**
 * @brief Class responsible for rendering models and managing lights and
 * shadows.
 *
 * This class manages a collection of render models and lights, allowing for the
 * rendering of 3D scenes with shadow mapping.
 */
class Renderer : public Logger {
  private:
    /** @brief List of render models */
    std::vector<RenderModel*> m_render_models;
    /** @brief Shader used for rendering */
    raylib::Shader m_shader;
    /** @brief Skybox shader */
    raylib::Shader m_skybox_shader;
    /** @brief Camera position uniform */
    ShaderUniform<raylib::Vector3> m_camera_position;
    /** @brief Ambient light coefficient uniform */
    ShaderUniform<float> m_ka;
    /** @brief List of lights */
    std::vector<std::shared_ptr<Light>> m_lights;
    /** @brief Shadow map resolution uniform */
    ShaderUniform<int> m_shadow_map_res;
    /** @brief Skybox model */
    std::shared_ptr<raylib::Model> m_skybox_model = NULL;
    /** @brief Skybox enabled flag */
    bool m_skybox_enabled = true;

    void draw_skybox(raylib::Vector3 camera_position) {
        if (!m_skybox_enabled)
            return;
        if (!m_skybox_model)
            return;
        rlDisableBackfaceCulling();
        rlDisableDepthMask();
        m_skybox_model->Draw(camera_position);
        rlEnableBackfaceCulling();
        rlEnableDepthMask();
    }

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
          m_skybox_shader(
              raylib::Shader("/Users/humzaqureshi/GitHub/"
                             "Super-Powered-Robot-Football/src/skybox.vs",
                             "/Users/humzaqureshi/GitHub/"
                             "Super-Powered-Robot-Football/src/skybox.fs")),
          m_camera_position("camPos", raylib::Vector3(0, 0, 0), m_shader),
          m_ka("ka", ka, m_shader),
          m_shadow_map_res("shadowMapRes", shadow_scale, m_shader) {}

    ~Renderer() {
        for (auto i : m_render_models) {
            delete i;
        }
    }

    template <typename... Args> RenderModel* create_render_model(Args... args) {
        m_render_models.push_back(new RenderModel(args...));
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
            light->BeginShadowMode();
            ClearBackground(BLACK);

            for (auto& i : m_render_models) {
                i->draw(light->light_cam().GetMatrix());
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
    void render(raylib::Camera* camera) {
        m_camera_position.value(camera->GetPosition());
        camera->BeginMode();
        ClearBackground(WHITE);
        game_info.visible_meshes = 0;
        draw_skybox(camera->GetPosition());
        for (auto& i : m_render_models) {
            i->draw(m_shader, camera->GetMatrix());
            i->clear_instances();
        }
        camera->EndMode();
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

    /** @brief Adds a skybox to the renderer
     *
     * @param path path to the skybox
     */
    void load_skybox(std::string path) {
        m_skybox_model =
            std::make_shared<raylib::Model>(raylib::Mesh::Cube(1, 1, 1));
        m_skybox_model->materials[0].shader = m_skybox_shader;
        SetShaderValue(m_skybox_model->materials[0].shader,
                       GetShaderLocation(m_skybox_model->materials[0].shader,
                                         "environmentMap"),
                       (int[1]){MATERIAL_MAP_CUBEMAP}, SHADER_UNIFORM_INT);
        raylib::Image img(path);
        m_skybox_model->materials[0].maps[MATERIAL_MAP_CUBEMAP].texture =
            LoadTextureCubemap(img, CUBEMAP_LAYOUT_AUTO_DETECT);
    }

    /** @brief Unloads the skybox from the renderer */
    void unload_skybox() { m_skybox_model = NULL; }

    /** @brief Enables the skybox */
    void enable_skybox() { m_skybox_enabled = true; }

    /** @brief Disables the skybox */
    void disable_skybox() { m_skybox_enabled = false; }
};

} // namespace SPRF

#endif // _SPRF_RENDERER_HPP_