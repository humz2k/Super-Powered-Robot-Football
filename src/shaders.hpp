#ifndef _SPRF_SHADERS_HPP_
#define _SPRF_SHADERS_HPP_

#define MAX_LIGHTS 4

#include "raylib-cpp.hpp"
#include "shadow_map_texture.hpp"
#include <string>

namespace SPRF {

static const char* lights_vs = "#version 330\n"
                               "in vec3 vertexPosition; \
in vec2 vertexTexCoord; \
in vec3 vertexNormal; \
in vec4 vertexColor; \
uniform mat4 mvp; \
uniform mat4 matModel; \
uniform mat4 matNormal; \
out vec3 fragPosition; \
out vec2 fragTexCoord; \
out vec4 fragColor; \
out vec3 fragNormal; \
void main() { \
    fragPosition = vec3(matModel*vec4(vertexPosition, 1.0)); \
    fragTexCoord = vertexTexCoord; \
    fragColor = vertexColor; \
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0))); \
    gl_Position = mvp*vec4(vertexPosition, 1.0); \
}\n";

template <class T>
class ShaderUniformBase{
    protected:
        T m_value;
        raylib::Shader& m_shader;
        int m_loc;
    public:
        virtual void update_value(){}
        ShaderUniformBase(std::string name, T value, raylib::Shader& shader) : m_value(value), m_shader(shader), m_loc(m_shader.GetLocation(name)){update_value();}
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
        using ShaderUniformBase::ShaderUniformBase;
        void update_value(){
            shader().SetValue(loc(),&this->m_value,SHADER_UNIFORM_INT);
        }
};

template<>
class ShaderUniform<float> : public ShaderUniformBase<float>{
    public:
        using ShaderUniformBase::ShaderUniformBase;
        void update_value(){
            shader().SetValue(loc(),&this->m_value,SHADER_UNIFORM_FLOAT);
        }
};

template<>
class ShaderUniform<raylib::Color> : public ShaderUniformBase<raylib::Color>{
    public:
        using ShaderUniformBase::ShaderUniformBase;
        void update_value(){
            float array[4] = {
            (float)value().r / (float)255.0f, (float)value().g / (float)255.0f,
            (float)value().b / (float)255.0f, (float)value().a / (float)255.0f};
            shader().SetValue(loc(), array, SHADER_ATTRIB_VEC4);
        }
};

template<>
class ShaderUniform<raylib::Vector2> : public ShaderUniformBase<raylib::Vector2>{
    public:
        using ShaderUniformBase::ShaderUniformBase;
        void update_value(){
            float array[3] = {value().x,value().y};
            shader().SetValue(loc(), array, SHADER_ATTRIB_VEC2);
        }
};

template<>
class ShaderUniform<raylib::Vector3> : public ShaderUniformBase<raylib::Vector3>{
    public:
        using ShaderUniformBase::ShaderUniformBase;
        void update_value(){
            float array[3] = {value().x,value().y,value().z};
            shader().SetValue(loc(), array, SHADER_ATTRIB_VEC3);
        }
};

template<>
class ShaderUniform<raylib::Vector4> : public ShaderUniformBase<raylib::Vector4>{
    public:
        using ShaderUniformBase::ShaderUniformBase;
        void update_value(){
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
    Light(raylib::Shader& shader, int width = 1024, int height = 1024, float scale = 10.0f, float fov = 20.0f)
        : m_id(light_count), m_shader(shader),
            m_enabled("lights[" + std::to_string(m_id) + "].enabled",0,m_shader),
            m_type("lights[" + std::to_string(m_id) + "].type",0,m_shader),
            m_kd("lights[" + std::to_string(m_id) + "].kd",0.5,m_shader),
            m_ks("lights[" + std::to_string(m_id) + "].ks",0.2,m_shader),
            m_p("lights[" + std::to_string(m_id) + "].p",200,m_shader),
            m_intensity("lights[" + std::to_string(m_id) + "].intensity",1,m_shader),
            m_cL("lights[" + std::to_string(m_id) + "].cL",WHITE,m_shader),
            m_pos("lights[" + std::to_string(m_id) + "].pos",raylib::Vector3(0),m_shader),
            m_L("lights[" + std::to_string(m_id) + "].L",raylib::Vector3(1,1,1),m_shader),
          m_shadow_map(LoadShadowmapRenderTexture(width, height)),
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
        return m_L.value(v);
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
        out.target = raylib::Vector3(0);
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

    void EndShadowMode(int slot_start = 2){
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

static const char* lights_fs = "#version 330\n"
                               "in vec3 fragPosition; \
in vec2 fragTexCoord; \
in vec4 fragColor; \
in vec3 fragNormal; \
uniform sampler2D texture0; \
uniform vec4 colDiffuse; \
uniform vec3 camPos; \
uniform float ka; \
uniform int shadowMapRes; \
out vec4 finalColor; \n"
                               "#define MAX_LIGHTS 4\n"
                               "struct Light { \
    int enabled; \
    int type; \
    vec3 cL; \
    float kd; \
    float ks; \
    vec3 pos; \
    vec3 L; \
    float p; \
    float intensity; \
    sampler2D shadowMap; \
};\n\
uniform mat4 light_vp[MAX_LIGHTS];\
uniform Light lights[MAX_LIGHTS];\
vec3 diffuse(vec3 cM, vec3 cL, vec3 N, vec3 L){ \
    return cM*cL*max(0,dot(N,L)); \
} \
vec3 specular(vec3 cL, vec3 N, vec3 H, float p){ \
    return cL*pow(max(0,dot(N,H)),p); \
} \
vec3 calculate_light(Light light, vec3 cM, vec3 N, vec3 V){\
    vec3 H = normalize(V + light.L); \
    vec3 cL = light.cL; \
    vec3 L = light.L; \
    float kd = light.kd; \
    float ks = light.ks; \
    float p = light.p; \
    vec3 temp = kd * diffuse(cM,cL,N,L) + ks * specular(cL,N,H,p); \
    return temp; \
}\n"
                               "void main() \
{ \
    vec4 base_color = texture(texture0, fragTexCoord) * colDiffuse; \
    vec3 cM = base_color.xyz; \
    vec3 out_col = cM * ka; \
    vec3 N = fragNormal; \
    vec3 V = normalize(camPos - fragPosition); \
    for (int i = 0; i < MAX_LIGHTS; i++){ \
        if (lights[i].enabled == 0)continue;\
        out_col += calculate_light(lights[i],cM,N,V);\
    } \
    finalColor = vec4(out_col.x,out_col.y,out_col.z,base_color.w); \
    finalColor = pow(finalColor, vec4(1.0/2.2)); \
}\n";

} // namespace SPRF

/*static const char* lights_fs = "#version 330\n"
"in vec3 fragPosition; \
in vec2 fragTexCoord; \
in vec4 fragColor; \
in vec3 fragNormal; \
uniform sampler2D texture0; \
uniform vec4 colDiffuse; \
uniform vec3 camPos; \
uniform float ka; \
uniform int shadowMapRes; \
out vec4 finalColor;\n"
"#define FLUX_MAX_LIGHTS 2\n"
"struct Light { \
    int enabled; \
    int type; \
    vec3 cL; \
    float kd; \
    float ks; \
    vec3 pos; \
    vec3 L; \
    float p; \
    float intensity; \
    sampler2D shadowMap; \
};"
"uniform mat4 light_vp[FLUX_MAX_LIGHTS]; \
uniform Light lights[FLUX_MAX_LIGHTS]; \
vec3 diffuse(vec3 cM, vec3 cL, vec3 N, vec3 L){ \
    return cM*cL*max(0,dot(N,L)); \
} \
vec3 specular(vec3 cL, vec3 N, vec3 H, float p){ \
    return cL*pow(max(0,dot(N,H)),p); \
} \
vec3 calculate_light(Light light, mat4 vp, vec3 cM, vec3 N, vec3 V){\
    vec3 H = normalize(V + light.L); \
    vec3 cL = light.cL; \
    vec3 L = light.L; \
    float kd = light.kd; \
    float ks = light.ks; \
    float p = light.p; \
    vec3 temp = kd * diffuse(cM,cL,N,L) + ks * specular(cL,N,H,p); \
    vec4 fragPosLightSpace = vp * vec4(fragPosition, 1); \
    fragPosLightSpace.xyz /= fragPosLightSpace.w;\
    fragPosLightSpace.xyz = (fragPosLightSpace.xyz + 1.0f) / 2.0f; \
    vec2 sampleCoords = fragPosLightSpace.xy;\
    float curDepth = fragPosLightSpace.z;\
    float bias = max(0.0002 * (1.0 - dot(N, L)), 0.00002) + 0.00001; \
    int shadowCounter = 0; \
    int sample_factor = 2; \
    int numSamples = (sample_factor*2+1);//*(sample_factor*2+1); \
    numSamples = numSamples*numSamples; \
    vec2 texelSize = vec2(1.0f / float(shadowMapRes)); \
    for (int x = -sample_factor; x <= sample_factor; x++) \
    { \
        for (int y = -sample_factor; y <= sample_factor; y++) \
        { \
            float sampleDepth = texture(light.shadowMap, sampleCoords +
texelSize * vec2(x, y)).r; \
            if (curDepth - bias > sampleDepth) \
            { \
                shadowCounter++; \
            } \
        } \
    } \
    float shadow = float(shadowCounter)/float(numSamples); \
    temp = mix(vec4(temp,1), vec4(0, 0, 0, 1), shadow).xyz; \
    return temp; \
}\
void main() \
{ \
    vec4 base_color = texture(texture0, fragTexCoord) * colDiffuse; \
    vec3 cM = base_color.xyz; \
    vec3 N = fragNormal; \
    vec3 V = normalize(camPos - fragPosition); \
    vec3 out_col = cM * ka; \
    for (int i = 0; i < FLUX_MAX_LIGHTS; i++){ \
        if (lights[i].enabled == 1){ \
            out_col += calculate_light(lights[i],light_vp[i],cM,N,V); \
        } \
    } \
    finalColor = vec4(out_col.x,out_col.y,out_col.z,base_color.w); \
    finalColor = pow(finalColor, vec4(1.0/2.2)); \
}\n";*/

#endif // _SPRF_SHADERS_HPP_