#ifndef _SPRF_SHADERS_HPP_
#define _SPRF_SHADERS_HPP_

#define MAX_LIGHTS 2

#include "raylib-cpp.hpp"
#include "shadow_map_texture.hpp"
#include <string>

namespace SPRF {

template <class T>
class ShaderUniformBase{
    protected:
        T m_value;
        raylib::Shader& m_shader;
        int m_loc;
    public:
        virtual void update_value(){}
        ShaderUniformBase(std::string name, T value, raylib::Shader& shader) : m_value(value), m_shader(shader), m_loc(m_shader.GetLocation(name)){}
        T value() const{return m_value;}
        T value(T value){m_value = value; update_value(); return m_value;}
        int loc(){return m_loc;}
        raylib::Shader& shader(){return m_shader;}
        ShaderUniformBase& operator=(const T& v){
            value(v);
        }
};

template <class T>
class ShaderUniform;

template<>
class ShaderUniform<int> : public ShaderUniformBase<int>{
    public:
        ShaderUniform(std::string name, int value, raylib::Shader& shader) : ShaderUniformBase(name,value,shader){
            update_value();
        }
        void update_value() override{
            shader().SetValue(loc(),&this->m_value,SHADER_UNIFORM_INT);
        }
};

template<>
class ShaderUniform<float> : public ShaderUniformBase<float>{
    public:
        ShaderUniform(std::string name, float value, raylib::Shader& shader) : ShaderUniformBase(name,value,shader){
            update_value();
        }
        void update_value() override{
            shader().SetValue(loc(),&this->m_value,SHADER_UNIFORM_FLOAT);
        }
};

template<>
class ShaderUniform<raylib::Color> : public ShaderUniformBase<raylib::Color>{
    public:
        ShaderUniform(std::string name, raylib::Color value, raylib::Shader& shader) : ShaderUniformBase(name,value,shader){
            update_value();
        }
        void update_value() override{
            float array[4] = {
            ((float)value().r) / (float)255.0f, ((float)value().g) / (float)255.0f,
            ((float)value().b) / (float)255.0f, ((float)value().a) / (float)255.0f};
            std::cout << "color: " << array[0] << ", " << array[1] << ", " << array[2] << ", " << array[3] << std::endl;
            shader().SetValue(loc(), array, SHADER_ATTRIB_VEC3);
        }
};

template<>
class ShaderUniform<raylib::Vector2> : public ShaderUniformBase<raylib::Vector2>{
    public:
        ShaderUniform(std::string name, raylib::Vector2 value, raylib::Shader& shader) : ShaderUniformBase(name,value,shader){
            update_value();
        }
        void update_value() override{
            float array[3] = {value().x,value().y};
            shader().SetValue(loc(), array, SHADER_ATTRIB_VEC2);
        }
};

template<>
class ShaderUniform<raylib::Vector3> : public ShaderUniformBase<raylib::Vector3>{
    public:
        ShaderUniform(std::string name, raylib::Vector3 value, raylib::Shader& shader) : ShaderUniformBase(name,value,shader){
            update_value();
        }
        void update_value() override{
            float array[3] = {value().x,value().y,value().z};
            shader().SetValue(loc(), array, SHADER_ATTRIB_VEC3);
        }
};

template<>
class ShaderUniform<raylib::Vector4> : public ShaderUniformBase<raylib::Vector4>{
    public:
        ShaderUniform(std::string name, raylib::Vector4 value, raylib::Shader& shader) : ShaderUniformBase(name,value,shader){
            update_value();
        }
        void update_value() override{
            float array[4] = {value().x,value().y,value().z,value().w};
            shader().SetValue(loc(), array, SHADER_ATTRIB_VEC4);
        }
};

class Light {
  private:
    inline static int light_count = 0;
    int m_id;
    raylib::Shader& m_shader;
    ShaderUniform<int> m_enabled;
    ShaderUniform<int> m_type;
    ShaderUniform<float> m_kd;
    ShaderUniform<float> m_ks;
    ShaderUniform<float> m_p;
    ShaderUniform<float> m_intensity;
    ShaderUniform<raylib::Color> m_cL;
    ShaderUniform<raylib::Vector3> m_pos;
    ShaderUniform<raylib::Vector3> m_L;

    raylib::Matrix m_light_view;
    raylib::Matrix m_light_proj;

    float m_scale;
    float m_fov;

    int m_shadow_mapLoc;
    int m_light_vpLoc;

    RenderTexture2D m_shadow_map;
    raylib::Matrix m_light_vp;

  public:
    Light(raylib::Shader& shader, int shadowMapRes = 2048, float scale = 20.0f, float fov = 10.0f)
        : m_id(light_count), m_shader(shader),
            m_enabled("lights[" + std::to_string(m_id) + "].enabled",0,m_shader),
            m_type("lights[" + std::to_string(m_id) + "].type",0,m_shader),
            m_kd("lights[" + std::to_string(m_id) + "].kd",0.5,m_shader),
            m_ks("lights[" + std::to_string(m_id) + "].ks",0.2,m_shader),
            m_p("lights[" + std::to_string(m_id) + "].p",200,m_shader),
            m_intensity("lights[" + std::to_string(m_id) + "].intensity",1,m_shader),
            m_cL("lights[" + std::to_string(m_id) + "].cL",raylib::Color::White(),m_shader),
            m_pos("lights[" + std::to_string(m_id) + "].pos",raylib::Vector3(0),m_shader),
            m_L("lights[" + std::to_string(m_id) + "].L",raylib::Vector3(1,1,1),m_shader),
          m_shadow_map(LoadShadowmapRenderTexture(shadowMapRes, shadowMapRes)),
          m_scale(scale), m_fov(fov), m_shadow_mapLoc(shader.GetLocation("lights[" + std::to_string(m_id) + "].shadowMap")), m_light_vpLoc(shader.GetLocation("light_vp[" + std::to_string(m_id) + "]")) {
            light_count++;
            std::cout << "light id = " << m_id << std::endl;
    }

    int enabled() const{
        return m_enabled.value();
    }

    int enabled(int v){
        return m_enabled.value(v);
    }

    int type() const{
        return m_type.value();
    }

    int type(int v){
        return m_type.value(v);
    }

    float kd() const{
        return m_kd.value();
    }

    float kd(float v){
        return m_kd.value(v);
    }

    float ks() const{
        return m_ks.value();
    }

    float ks(float v){
        return m_ks.value(v);
    }

    float p() const{
        return m_p.value();
    }

    float p(float v){
        return m_p.value(v);
    }

    float intensity() const{
        return m_intensity.value();
    }

    float intensity(float v){
        return m_intensity.value(v);
    }

    raylib::Color cL() const{
        return m_cL.value();
    }

    raylib::Color cL(raylib::Color v){
        return m_cL.value(v);
    }

    raylib::Vector3 pos() const{
        return m_pos.value();
    }

    raylib::Vector3 pos(raylib::Vector3 v){
        return m_pos.value(v);
    }

    raylib::Vector3 L() const{
        return m_L.value();
    }

    raylib::Vector3 L(raylib::Vector3 v){
        return m_L.value(v.Normalize());
    }

    float scale() const{
        return m_scale;
    }

    float fov() const{
        return m_fov;
    }

    int id() const{
        return m_id;
    }

    ~Light() { UnloadShadowmapRenderTexture(m_shadow_map); }

    raylib::Camera3D light_cam() const{
        raylib::Camera3D out;
        out.position = L() * scale();
        out.projection = CAMERA_ORTHOGRAPHIC;
        out.target = raylib::Vector3(0,0,0);
        out.fovy = fov();
        out.up = raylib::Vector3(0.0f,1.0f,0.0f);
        return out;
    }

    void BeginShadowMode(){
        BeginTextureMode(m_shadow_map);
        ClearBackground(WHITE);
        BeginMode3D(light_cam());
        m_light_view = rlGetMatrixModelview();
        m_light_proj = rlGetMatrixProjection();
    }

    void EndShadowMode(int slot_start){
        EndMode3D();
        EndTextureMode();
        raylib::Matrix light_view_proj = m_light_view * m_light_proj;
        SetShaderValueMatrix(m_shader,m_light_vpLoc,light_view_proj);
        rlEnableShader(m_shader.id);
        int slot = slot_start + id();
        rlActiveTextureSlot(slot);
        rlEnableTexture(m_shadow_map.depth.id);
        rlSetUniform(m_shadow_mapLoc, &slot, SHADER_UNIFORM_INT,
                     1);
    }
};

} // namespace SPRF

#endif // _SPRF_SHADERS_HPP_