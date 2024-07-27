#ifndef _SPRF_RENDERER_HPP_
#define _SPRF_RENDERER_HPP_

#include "base.hpp"
#include "raylib-cpp.hpp"
#include "shaders.hpp"
#include "shadow_map_texture.hpp"
#include "shader_sources.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace SPRF {

/**
 * @brief Represents a plane in 3D space.
 */
struct Plane {
    /** @brief Normal vector of the plane */
    raylib::Vector3 normal = raylib::Vector3(0, 1, 0);
    /** @brief A point on the plane */
    raylib::Vector3 point = raylib::Vector3(0, 0, 0);

    /**
     * @brief Calculate the signed distance from a point to the plane.
     * @param P The point to calculate the distance from.
     * @return Signed distance from the point to the plane.
     */
    float signed_distance(Vector3 P) {
        return normal.DotProduct(Vector3Subtract(P, point));
    }
};

/**
 * @brief Represents the view frustum of a camera.
 */
class ViewFrustrum {
  private:
    /** @brief Reference to the camera */
    raylib::Camera3D& m_camera;
    /** @brief Planes defining the view frustum */
    Plane m_planes[6];

    /**
     * @brief Get the display height.
     * @return Display height.
     */
    float display_height() {
        return IsWindowFullscreen() ? GetRenderHeight() : GetScreenHeight();
    }

    /**
     * @brief Get the display width.
     * @return Display width.
     */
    float display_width() {
        return IsWindowFullscreen() ? GetRenderWidth() : GetScreenWidth();
    }

    /**
     * @brief Get the aspect ratio.
     * @return Aspect ratio.
     */
    float aspect() { return display_width() / display_height(); }

    /**
     * @brief Get the field of view.
     * @return Field of view.
     */
    float fov_y() { return m_camera.fovy; }

    /**
     * @brief Get the near clipping plane distance.
     * @return Near clipping plane distance.
     */
    float z_near() { return RL_CULL_DISTANCE_NEAR; }

    /**
     * @brief Get the far clipping plane distance.
     * @return Far clipping plane distance.
     */
    float z_far() { return RL_CULL_DISTANCE_FAR; }

    /**
     * @brief Get the camera position.
     * @return Camera position.
     */
    raylib::Vector3 cam_position() { return m_camera.position; }

    /**
     * @brief Get the camera front vector.
     * @return Camera front vector.
     */
    raylib::Vector3 cam_front() {
        return (raylib::Vector3(m_camera.target) -
                raylib::Vector3(m_camera.position))
            .Normalize();
    }

    /**
     * @brief Get the camera up vector.
     * @return Camera up vector.
     */
    raylib::Vector3 cam_up() { return m_camera.up; }

    /**
     * @brief Get the camera right vector.
     * @return Camera right vector.
     */
    raylib::Vector3 cam_right() { return cam_front().CrossProduct(cam_up()); }

  public:
    /**
     * @brief Construct a new ViewFrustrum object.
     * @param camera Reference to the camera.
     */
    ViewFrustrum(raylib::Camera3D& camera) : m_camera(camera) {
        float halfVSide = z_far() * tanf(fov_y() * 0.5f);
        float halfHSide = halfVSide * aspect();
        auto front = cam_front();
        auto frontMultFar = front * z_far();

        m_planes[0].normal = front;
        m_planes[0].point = cam_position() + (front * z_near());
        m_planes[1].normal = -front;
        m_planes[1].point = cam_position() + (frontMultFar);
        m_planes[2].normal =
            (frontMultFar - cam_right() * halfHSide).CrossProduct(cam_up());
        m_planes[2].point = cam_position();
        m_planes[3].normal =
            cam_up().CrossProduct(frontMultFar + cam_right() * halfHSide);
        m_planes[3].point = cam_position();
        m_planes[4].normal =
            cam_right().CrossProduct(frontMultFar - cam_up() * halfVSide);
        m_planes[4].point = cam_position();
        m_planes[5].normal =
            (frontMultFar + cam_up() * halfVSide).CrossProduct(cam_right());
        m_planes[5].point = cam_position();
    }

    /**
     * @brief Check if a point is inside the view frustum.
     * @param point The point to check.
     * @return True if the point is inside the view frustum, false otherwise.
     */
    bool point_inside(Vector3 point, float radius = 0) {
        for (int i = 0; i < 6; i++) {
            if (m_planes[i].signed_distance(point) < -radius) {
                return false;
            }
        }
        return true;
    }
};

/**
 * @brief Represents a bounding box with corner points.
 */
struct BBoxCorners {
    /** @brief Corner points of the bounding box */
    raylib::Vector3 c1, c2, c3, c4, c5, c6, c7, c8;
    float m_radius;

    /**
     * @brief Construct a new BBox object from a BoundingBox.
     * @param bbox BoundingBox to construct from.
     */
    BBoxCorners(BoundingBox bbox) {
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

        auto diff = Vector3Subtract(bbox.max, bbox.min);
        diff.x = fabs(diff.x);
        diff.y = fabs(diff.y);
        diff.z = fabs(diff.z);
        m_radius = diff.x;
        if (diff.y > m_radius) {
            m_radius = diff.y;
        }
        if (diff.z > m_radius) {
            m_radius = diff.z;
        }
    }

    BBoxCorners() {}

    /**
     * @brief Check if the bounding box is visible with a vp matrix.
     * @param matrix vp matrix.
     * @return True if visible, false otherwise.
     */
    bool visible(raylib::Matrix vp) {
        Vector3* points = (Vector3*)this;
        for (int i = 0; i < 8; i++) {
            if (Vector3Transform(points[i], vp).z <= 0)
                return true;
        }
        return false;
    }

    /**
     * @brief Check if the bounding box is visible with a transformation matrix
     * and view frustum.
     * @param transform Transformation matrix.
     * @param frustrum View frustum.
     * @return True if visible, false otherwise.
     */
    bool visible(Matrix& transform, ViewFrustrum& frustrum) {
        Vector3* points = (Vector3*)this;
        for (int i = 0; i < 8; i++) {
            if (frustrum.point_inside(Vector3Transform(points[i], transform))) {
                return true;
            }
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
    /** @brief Pointer to the model */
    raylib::Model* m_model;
    /** @brief Bounding boxes of meshes of the model */
    BBoxCorners* m_bounding_boxes = NULL;
    /** @brief Transformation matrix of the model */
    raylib::Matrix m_model_transform;
    /** @brief Number of instances allocated */
    int m_instances_allocated = 0;
    /** @brief Number of instances of the model */
    int m_n_instances = 0;
    /** @brief Instances of the model */
    raylib::Matrix* m_instances = NULL;
    /** @brief Number of visible instances of the model */
    int m_n_visible_instances = 0;
    /** @brief Visible instances of the model */
    Matrix* m_visible_instances = NULL;

    bool m_clip = true;

    /**
     * @brief Reallocate memory for instances.
     */
    void realloc_instances() {
        m_instances = (raylib::Matrix*)realloc(
            m_instances, sizeof(raylib::Matrix) * m_instances_allocated);
        m_visible_instances = (Matrix*)realloc(
            m_visible_instances, sizeof(Matrix) * m_instances_allocated);
    }

  public:
    /**
     * @brief Construct a new RenderModel object.
     *
     * @tparam Args Parameter pack for the model constructor.
     * @param args Arguments for the model constructor.
     */
    template <typename... Args>
    RenderModel(Args... args)
        : m_model(new raylib::Model(args...)),
          m_model_transform(raylib::Matrix(m_model->GetTransform())) {
        m_bounding_boxes =
            (BBoxCorners*)malloc(sizeof(BBoxCorners) * m_model->meshCount);
        for (int i = 0; i < m_model->meshCount; i++) {
            m_bounding_boxes[i] =
                (BBoxCorners(GetMeshBoundingBox(m_model->meshes[i])));
        }
        m_instances_allocated = 50;
        realloc_instances();
    }

    ~RenderModel() {
        delete m_model;
        free(m_bounding_boxes);
        free(m_instances);
        free(m_visible_instances);
    }

    /**
     * @brief Sets whether this model should be frustrum culled
     *
     * @param clip whether this model should be frustrum culled
     * @return whether this model should be frustrum culled
     */
    bool clip(bool clip) {
        m_clip = clip;
        return m_clip;
    }

    /**
     * @brief Gets whether this model should be frustrum culled
     *
     * @return whether this model should be frustrum culled
     */
    bool clip() { return m_clip; }

    /**
     * @brief Clear all instances of the model.
     */
    void clear_instances() { m_n_instances = 0; }

    /**
     * @brief Add an instance of the model.
     *
     * @param instance Transformation matrix of the instance.
     */
    void add_instance(raylib::Matrix instance) {
        if (m_n_instances == m_instances_allocated) {
            m_instances_allocated *= 2;
            realloc_instances();
        }
        m_instances[m_n_instances] = m_model_transform * instance;
        m_n_instances++;
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
        for (int i = 0; i < m_model->meshCount; i++) {

            BBoxCorners bbox = m_bounding_boxes[i];
            m_n_visible_instances = 0;

            raylib::Matrix* instance_ptr = m_instances;
            for (int j = 0; j < m_n_instances; j++) {
                raylib::Matrix transform = *instance_ptr++;
                if (bbox.visible(transform * vp)) {
                    game_info.visible_meshes++;
                    m_visible_instances[m_n_visible_instances] = transform;
                    m_n_visible_instances++;
                } else {
                    game_info.hidden_meshes++;
                }
            }

            Mesh mesh = m_model->meshes[i];
            Material material = m_model->materials[m_model->meshMaterial[i]];
            material.shader = shader;
            DrawMeshInstanced(mesh, material, m_visible_instances,
                              m_n_visible_instances);
            material.shader = old_shader;
        }
    }

    /**
     * @brief Draw the model with a specified shader and view frustum.
     *
     * This method temporarily sets the shader of the model's materials to the
     * provided shader, draws all instances of the model, and then restores the
     * original shader.
     *
     * @param shader Shader to be used for drawing.
     * @param vp View-projection matrix.
     * @param frustrum View frustum.
     */
    void draw(Shader shader, raylib::Matrix vp, ViewFrustrum& frustrum) {
        if (!m_clip) {
            draw(shader, vp);
            return;
        }
        Shader old_shader = m_model->materials[0].shader;
        for (int i = 0; i < m_model->meshCount; i++) {

            BBoxCorners bbox = m_bounding_boxes[i];
            m_n_visible_instances = 0;

            raylib::Matrix* instance_ptr = m_instances;
            for (int j = 0; j < m_n_instances; j++) {
                raylib::Matrix transform = *instance_ptr++;
                if (bbox.visible(transform, frustrum)) {
                    game_info.visible_meshes++;
                    m_visible_instances[m_n_visible_instances] = transform;
                    m_n_visible_instances++;
                } else {
                    game_info.hidden_meshes++;
                }
            }

            Mesh mesh = m_model->meshes[i];
            Material material = m_model->materials[m_model->meshMaterial[i]];
            material.shader = shader;
            DrawMeshInstanced(mesh, material, m_visible_instances,
                              m_n_visible_instances);
            material.shader = old_shader;
        }
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
    /** @brief Shader used for rendering shadows */
    raylib::Shader m_shadow_shader;
    /** @brief Skybox shader */
    raylib::Shader m_skybox_shader;
    /** @brief Camera position uniform */
    ShaderUniform<raylib::Vector3> m_camera_position;
    /** @brief Ambient light coefficient uniform */
    ShaderUniform<float> m_ka;
    /** @brief List of lights */
    std::vector<Light*> m_lights;
    /** @brief Shadow map resolution uniform */
    ShaderUniform<int> m_shadow_map_res;
    /** @brief Skybox model */
    std::shared_ptr<raylib::Model> m_skybox_model = NULL;
    /** @brief Skybox enabled flag */
    bool m_skybox_enabled = true;

    /**
     * @brief Draw the skybox.
     * @param camera_position Position of the camera.
     */
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
    Renderer(float ka = 0.2, int shadow_scale = 4096)
        : m_shader(
              raylib::Shader::LoadFromMemory(lights_vs,
                             lights_fs)),
          m_shadow_shader(
              raylib::Shader::LoadFromMemory(lights_vs,
                             base_fs)),
          m_skybox_shader(
              raylib::Shader::LoadFromMemory(skybox_vs,
                             skybox_fs)),
          m_camera_position("camPos", raylib::Vector3(0, 0, 0), m_shader),
          m_ka("ka", ka, m_shader),
          m_shadow_map_res("shadowMapRes", shadow_scale, m_shader) {
        m_shader.locs[SHADER_LOC_MATRIX_MVP] =
            GetShaderLocation(m_shader, "mvp");
        m_shader.locs[SHADER_LOC_MATRIX_MODEL] =
            GetShaderLocationAttrib(m_shader, "instanceTransform");
        m_shadow_shader.locs[SHADER_LOC_MATRIX_MVP] =
            GetShaderLocation(m_shadow_shader, "mvp");
        m_shadow_shader.locs[SHADER_LOC_MATRIX_MODEL] =
            GetShaderLocationAttrib(m_shadow_shader, "instanceTransform");
    }

    ~Renderer() {
        for (auto i : m_render_models) {
            delete i;
        }
        for (auto i : m_lights) {
            delete i;
        }
    }

    /**
     * @brief Create a render model and add it to the renderer.
     *
     * @tparam Args Parameter pack for the model constructor.
     * @param args Arguments for the model constructor.
     * @return Pointer to the created render model.
     */
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
     * @return Pointer to the created light.
     */
    Light* add_light() {
        Light* out = new Light(m_shader, m_shadow_map_res.value());
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
    void calculate_shadows(raylib::Camera* camera) {
        int slot_start = 15 - MAX_LIGHTS;
        assert(m_lights.size() <= MAX_LIGHTS);
        for (auto& light : m_lights) {
            if (!light->enabled())
                continue;
            light->BeginShadowMode(camera);
            ClearBackground(BLACK);

            for (auto& i : m_render_models) {
                i->draw(m_shadow_shader, light->light_cam(camera).GetMatrix());
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
    void render(raylib::Camera* camera,
                raylib::Color background_color = raylib::Color::White()) {
        m_camera_position.value(camera->GetPosition());
        ViewFrustrum frustrum(*camera);
        // auto cam = m_lights[0]->light_cam(camera);
        camera->BeginMode();
        // cam.BeginMode();
        ClearBackground(background_color);
        game_info.visible_meshes = 0;
        game_info.hidden_meshes = 0;
        draw_skybox(camera->GetPosition());
        for (auto& i : m_render_models) {
            i->draw(m_shader, camera->GetMatrix(), frustrum);
            // i->draw(m_shader, cam.GetMatrix());
            i->clear_instances();
        }
        // cam.EndMode();
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