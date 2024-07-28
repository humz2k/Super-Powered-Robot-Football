#ifndef _SPRF_SHADERS_HPP_
#define _SPRF_SHADERS_HPP_

#define MAX_LIGHTS 2

#include "base.hpp"
#include "raylib-cpp.hpp"
#include "shadow_map_texture.hpp"
#include <string>

namespace SPRF {

/**
 * @brief Base class for shader uniform variables.
 *
 * @tparam T Type of the uniform variable.
 */
template <class T> class ShaderUniformBase : public Logger {
  protected:
    /** @brief Value of the uniform variable */
    T m_value;
    /** @brief Reference to the shader */
    raylib::Shader& m_shader;
    /** @brief Location of the uniform variable in the shader */
    int m_loc;

  public:
    /**
     * @brief Default method to update the value of the uniform variable.
     */
    virtual void update_value() {}

    /**
     * @brief Construct a new Shader Uniform Base object.
     *
     * @param name Name of the uniform variable in the shader.
     * @param value Initial value of the uniform variable.
     * @param shader Reference to the shader.
     */
    ShaderUniformBase(std::string name, T value, raylib::Shader& shader)
        : m_value(value), m_shader(shader), m_loc(m_shader.GetLocation(name)) {}

    /**
     * @brief Get the current value of the uniform variable.
     *
     * @return Current value of the uniform variable.
     */
    T value() const { return m_value; }

    /**
     * @brief Set a new value for the uniform variable and update it.
     *
     * @param value New value to set.
     * @return Updated value of the uniform variable.
     */
    T value(T value) {
        m_value = value;
        update_value();
        return m_value;
    }

    /**
     * @brief Get the location of the uniform variable in the shader.
     *
     * @return Location of the uniform variable.
     */
    int loc() { return m_loc; }

    /**
     * @brief Get the reference to the shader.
     *
     * @return Reference to the shader.
     */
    raylib::Shader& shader() { return m_shader; }

    /**
     * @brief Assignment operator to set a new value for the uniform variable.
     *
     * @param v New value to set.
     * @return Reference to the ShaderUniformBase object.
     */
    ShaderUniformBase& operator=(const T& v) { value(v); }
};

template <class T> class ShaderUniform;

/**
 * @brief Template specialization for integer shader uniform variables.
 */
template <> class ShaderUniform<int> : public ShaderUniformBase<int> {
  public:
    /**
     * @brief Construct a new Shader Uniform object for integers.
     *
     * @param name Name of the uniform variable in the shader.
     * @param value Initial value of the uniform variable.
     * @param shader Reference to the shader.
     */
    ShaderUniform(std::string name, int value, raylib::Shader& shader)
        : ShaderUniformBase(name, value, shader) {
        update_value();
    }

    /**
     * @brief Update the value of the integer uniform variable in the shader.
     */
    void update_value() override {
        shader().SetValue(loc(), &this->m_value, SHADER_UNIFORM_INT);
    }
};

/**
 * @brief Template specialization for float shader uniform variables.
 */
template <> class ShaderUniform<float> : public ShaderUniformBase<float> {
  public:
    /**
     * @brief Construct a new Shader Uniform object for floats.
     *
     * @param name Name of the uniform variable in the shader.
     * @param value Initial value of the uniform variable.
     * @param shader Reference to the shader.
     */
    ShaderUniform(std::string name, float value, raylib::Shader& shader)
        : ShaderUniformBase(name, value, shader) {
        update_value();
    }

    /**
     * @brief Update the value of the float uniform variable in the shader.
     */
    void update_value() override {
        shader().SetValue(loc(), &this->m_value, SHADER_UNIFORM_FLOAT);
    }
};

/**
 * @brief Template specialization for Color shader uniform variables.
 */
template <>
class ShaderUniform<raylib::Color> : public ShaderUniformBase<raylib::Color> {
  public:
    /**
     * @brief Construct a new Shader Uniform object for colors.
     *
     * @param name Name of the uniform variable in the shader.
     * @param value Initial value of the uniform variable.
     * @param shader Reference to the shader.
     */
    ShaderUniform(std::string name, raylib::Color value, raylib::Shader& shader)
        : ShaderUniformBase(name, value, shader) {
        update_value();
    }

    /**
     * @brief Update the value of the color uniform variable in the shader.
     */
    void update_value() override {
        float array[4] = {((float)value().r) / (float)255.0f,
                          ((float)value().g) / (float)255.0f,
                          ((float)value().b) / (float)255.0f,
                          ((float)value().a) / (float)255.0f};
        shader().SetValue(loc(), array, SHADER_ATTRIB_VEC3);
    }
};

/**
 * @brief Template specialization for Vector2 shader uniform variables.
 */
template <>
class ShaderUniform<raylib::Vector2>
    : public ShaderUniformBase<raylib::Vector2> {
  public:
    /**
     * @brief Construct a new Shader Uniform object for Vector2.
     *
     * @param name Name of the uniform variable in the shader.
     * @param value Initial value of the uniform variable.
     * @param shader Reference to the shader.
     */
    ShaderUniform(std::string name, raylib::Vector2 value,
                  raylib::Shader& shader)
        : ShaderUniformBase(name, value, shader) {
        update_value();
    }

    /**
     * @brief Update the value of the Vector2 uniform variable in the shader.
     */
    void update_value() override {
        float array[3] = {value().x, value().y};
        shader().SetValue(loc(), array, SHADER_ATTRIB_VEC2);
    }
};

/**
 * @brief Template specialization for Vector3 shader uniform variables.
 */
template <>
class ShaderUniform<raylib::Vector3>
    : public ShaderUniformBase<raylib::Vector3> {
  public:
    /**
     * @brief Construct a new Shader Uniform object for Vector3.
     *
     * @param name Name of the uniform variable in the shader.
     * @param value Initial value of the uniform variable.
     * @param shader Reference to the shader.
     */
    ShaderUniform(std::string name, raylib::Vector3 value,
                  raylib::Shader& shader)
        : ShaderUniformBase(name, value, shader) {
        update_value();
    }

    /**
     * @brief Update the value of the Vector3 uniform variable in the shader.
     */
    void update_value() override {
        float array[3] = {value().x, value().y, value().z};
        shader().SetValue(loc(), array, SHADER_ATTRIB_VEC3);
    }
};

/**
 * @brief Template specialization for Vector4 shader uniform variables.
 */
template <>
class ShaderUniform<raylib::Vector4>
    : public ShaderUniformBase<raylib::Vector4> {
  public:
    /**
     * @brief Construct a new Shader Uniform object for Vector4.
     *
     * @param name Name of the uniform variable in the shader.
     * @param value Initial value of the uniform variable.
     * @param shader Reference to the shader.
     */
    ShaderUniform(std::string name, raylib::Vector4 value,
                  raylib::Shader& shader)
        : ShaderUniformBase(name, value, shader) {
        update_value();
    }

    /**
     * @brief Update the value of the Vector4 uniform variable in the shader.
     */
    void update_value() override {
        float array[4] = {value().x, value().y, value().z, value().w};
        shader().SetValue(loc(), array, SHADER_ATTRIB_VEC4);
    }
};

/**
 * @brief Class representing a light source in the scene.
 */
class Light : public Logger {
  private:
    /** @brief Static counter for light instances */
    inline static int light_count = 0;
    /** @brief ID of the light */
    int m_id;
    /** @brief Reference to the shader */
    raylib::Shader& m_shader;
    /** @brief Uniform variable for light enabled state */
    ShaderUniform<int> m_enabled;
    /** @brief Uniform variable for light type */
    ShaderUniform<int> m_type;
    /** @brief Uniform variable for diffuse coefficient */
    ShaderUniform<float> m_kd;
    /** @brief Uniform variable for specular coefficient */
    ShaderUniform<float> m_ks;
    /** @brief Uniform variable for specular power */
    ShaderUniform<float> m_p;
    /** @brief Uniform variable for light intensity */
    ShaderUniform<float> m_intensity;
    /** @brief Uniform variable for light color */
    ShaderUniform<raylib::Color> m_cL;
    /** @brief Uniform variable for light position */
    ShaderUniform<raylib::Vector3> m_pos;
    /** @brief Uniform variable for light direction */
    ShaderUniform<raylib::Vector3> m_L;
    /** @brief View matrix for light */
    raylib::Matrix m_light_view;
    /** @brief Projection matrix for light */
    raylib::Matrix m_light_proj;

    /** @brief Scale of the light */
    float m_scale;
    /** @brief Field of view for the light */
    float m_fov;

    /** @brief Location of the shadow map in the shader */
    int m_shadow_mapLoc;
    /** @brief Location of the light view-projection matrix in the shader */
    int m_light_vpLoc;

    /** @brief Shadow map texture */
    RenderTexture2D m_shadow_map;
    /** @brief Light view-projection matrix */
    // raylib::Matrix m_light_vp;

  public:
    /**
     * @brief Construct a new Light object.
     *
     * @param shader Reference to the shader.
     * @param shadowMapRes Resolution of the shadow map.
     * @param scale Scale of the light.
     * @param fov Field of view for the light.
     */
    Light(raylib::Shader& shader, int shadowMapRes = 2048, float scale = 20.0f,
          float fov = 10.0f)
        : m_id(light_count), m_shader(shader),
          m_enabled("lights[" + std::to_string(m_id) + "].enabled", 0,
                    m_shader),
          m_type("lights[" + std::to_string(m_id) + "].type", 0, m_shader),
          m_kd("lights[" + std::to_string(m_id) + "].kd", 0.5, m_shader),
          m_ks("lights[" + std::to_string(m_id) + "].ks", 0.2, m_shader),
          m_p("lights[" + std::to_string(m_id) + "].p", 200, m_shader),
          m_intensity("lights[" + std::to_string(m_id) + "].intensity", 1,
                      m_shader),
          m_cL("lights[" + std::to_string(m_id) + "].cL",
               raylib::Color::White(), m_shader),
          m_pos("lights[" + std::to_string(m_id) + "].pos", raylib::Vector3(0),
                m_shader),
          m_L("lights[" + std::to_string(m_id) + "].L",
              raylib::Vector3(1, 1, 1).Normalize(), m_shader),
          m_scale(scale), m_fov(fov),
          m_shadow_mapLoc(shader.GetLocation("lights[" + std::to_string(m_id) +
                                             "].shadowMap")),
          m_light_vpLoc(
              shader.GetLocation("light_vp[" + std::to_string(m_id) + "]")),
          m_shadow_map(LoadShadowmapRenderTexture(shadowMapRes, shadowMapRes)) {
        light_count++;
        assert(id() < MAX_LIGHTS);
    }

    /**
     * @brief Get the enabled state of the light.
     *
     * @return Enabled state of the light.
     */
    int enabled() const { return m_enabled.value(); }

    /**
     * @brief Set the enabled state of the light.
     *
     * @param v New enabled state.
     * @return Updated enabled state.
     */
    int enabled(int v) { return m_enabled.value(v); }

    /**
     * @brief Get the type of the light.
     *
     * @return Type of the light.
     */
    int type() const { return m_type.value(); }

    /**
     * @brief Set the type of the light.
     *
     * @param v New type.
     * @return Updated type.
     */
    int type(int v) { return m_type.value(v); }

    /**
     * @brief Get the diffuse coefficient of the light.
     *
     * @return Diffuse coefficient.
     */
    float kd() const { return m_kd.value(); }

    /**
     * @brief Set the diffuse coefficient of the light.
     *
     * @param v New diffuse coefficient.
     * @return Updated diffuse coefficient.
     */
    float kd(float v) { return m_kd.value(v); }

    /**
     * @brief Get the specular coefficient of the light.
     *
     * @return Specular coefficient.
     */
    float ks() const { return m_ks.value(); }

    /**
     * @brief Set the specular coefficient of the light.
     *
     * @param v New specular coefficient.
     * @return Updated specular coefficient.
     */
    float ks(float v) { return m_ks.value(v); }

    /**
     * @brief Get the specular power of the light.
     *
     * @return Specular power.
     */
    float p() const { return m_p.value(); }

    /**
     * @brief Set the specular power of the light.
     *
     * @param v New specular power.
     * @return Updated specular power.
     */
    float p(float v) { return m_p.value(v); }

    /**
     * @brief Get the intensity of the light.
     *
     * @return Intensity of the light.
     */
    float intensity() const { return m_intensity.value(); }

    /**
     * @brief Set the intensity of the light.
     *
     * @param v New intensity.
     * @return Updated intensity.
     */
    float intensity(float v) { return m_intensity.value(v); }

    /**
     * @brief Get the color of the light.
     *
     * @return Color of the light.
     */
    raylib::Color cL() const { return m_cL.value(); }

    /**
     * @brief Set the color of the light.
     *
     * @param v New color.
     * @return Updated color.
     */
    raylib::Color cL(raylib::Color v) { return m_cL.value(v); }

    /**
     * @brief Get the position of the light.
     *
     * @return Position of the light.
     */
    raylib::Vector3 pos() const { return m_pos.value(); }

    /**
     * @brief Set the position of the light.
     *
     * @param v New position.
     * @return Updated position.
     */
    raylib::Vector3 pos(raylib::Vector3 v) { return m_pos.value(v); }

    /**
     * @brief Get the direction of the light.
     *
     * @return Direction of the light.
     */
    raylib::Vector3 L() const { return m_L.value(); }

    /**
     * @brief Set the direction of the light.
     *
     * @param v New direction.
     * @return Updated direction.
     */
    raylib::Vector3 L(raylib::Vector3 v) { return m_L.value(v.Normalize()); }

    /**
     * @brief Get the scale of the light.
     *
     * @return Scale of the light.
     */
    float scale() const { return m_scale; }

    /**
     * @brief Get the field of view of the light.
     *
     * @return Field of view of the light.
     */
    float fov() const { return m_fov; }

    /**
     * @brief Get the ID of the light.
     *
     * @return ID of the light.
     */
    int id() const { return m_id; }

    /**
     * @brief Destroy the Light object.
     */
    ~Light() { UnloadShadowmapRenderTexture(m_shadow_map); }

    /**
     * @brief Get the camera representation of the light.
     *
     * @return Camera representation of the light.
     */
    raylib::Camera3D light_cam(raylib::Camera* camera) const {
        raylib::Camera3D out;
        out.position = L() * scale(); // + camera->GetPosition();
        out.projection = CAMERA_ORTHOGRAPHIC;
        out.target = raylib::Vector3(0, 0, 0); // camera->GetPosition();//
        out.fovy = fov();
        out.up = raylib::Vector3(0.0f, 1.0f, 0.0f);
        return out;
    }

    /**
     * @brief Begin shadow map rendering mode.
     */
    void BeginShadowMode(raylib::Camera* camera) {
        BeginTextureMode(m_shadow_map);
        ClearBackground(WHITE);
        BeginMode3D(light_cam(camera));
        m_light_view = rlGetMatrixModelview();
        m_light_proj = rlGetMatrixProjection();
    }

    /**
     * @brief End shadow map rendering mode and update the shader with the
     * shadow map texture.
     *
     * @param slot_start Starting texture slot for the shadow map.
     */
    void EndShadowMode(int slot_start) {
        EndMode3D();
        EndTextureMode();
        raylib::Matrix light_view_proj = m_light_view * m_light_proj;
        SetShaderValueMatrix(m_shader, m_light_vpLoc, light_view_proj);
        rlEnableShader(m_shader.id);
        int slot = slot_start + id();
        rlActiveTextureSlot(slot);
        rlEnableTexture(m_shadow_map.depth.id);
        rlSetUniform(m_shadow_mapLoc, &slot, SHADER_UNIFORM_INT, 1);
    }
};

} // namespace SPRF

#endif // _SPRF_SHADERS_HPP_