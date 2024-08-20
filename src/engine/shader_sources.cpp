#include "shader_sources.hpp"

const char* base_fs = R"(

#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
out vec4 finalColor;

void main(){
    vec4 texelColor = texture(texture0, fragTexCoord);
    finalColor = texelColor*colDiffuse;
}

)";

const char* lights_fs = R"(

#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 camPos;
uniform float ka;
uniform int shadowMapRes;

out vec4 finalColor;

#define MAX_LIGHTS 2

struct Light {
    int enabled;
    int type;
    vec3 cL;
    float kd;
    float ks;
    vec3 pos;
    vec3 L;
    float p;
    float intensity;
    sampler2D shadowMap;
};

uniform mat4 light_vp[MAX_LIGHTS];
uniform Light lights[MAX_LIGHTS];

vec3 diffuse(vec3 cM, vec3 cL, vec3 N, vec3 L){
    return cM*cL*max(0,dot(N,L));
}

vec3 specular(vec3 cL, vec3 N, vec3 H, float p){
    return cL*pow(max(0,dot(N,H)),p);
}

vec3 calculate_light(Light light, mat4 vp, vec3 cM, vec3 N, vec3 V){
    vec3 H = normalize(V + light.L);
    vec3 cL = light.cL;
    vec3 L = light.L;
    float kd = light.kd;
    float ks = light.ks;
    float p = light.p;
    vec3 temp = kd * diffuse(cM,cL,N,L) + ks * specular(cL,N,H,p);
    vec4 fragPosLightSpace = vp * vec4(fragPosition, 1);
    fragPosLightSpace.xyz /= fragPosLightSpace.w;
    fragPosLightSpace.xyz = (fragPosLightSpace.xyz + 1.0f) / 2.0f;
    vec2 sampleCoords = fragPosLightSpace.xy;
    float curDepth = fragPosLightSpace.z;
    float bias = max(0.0002 * (1.0 - dot(N, L)), 0.00002) + 0.00001;
    int shadowCounter = 0;int sample_factor = 2;
    int numSamples = (sample_factor*2+1);
    numSamples = numSamples*numSamples;
    vec2 texelSize = vec2(1.0f / float(shadowMapRes));
    for (int x = -sample_factor; x <= sample_factor; x++){
        for (int y = -sample_factor; y <= sample_factor; y++){
            float sampleDepth = texture(light.shadowMap, sampleCoords + texelSize * vec2(x, y)).r;
            if (curDepth - bias > sampleDepth){
                shadowCounter++;
            }
        }
    }
    float shadow = float(shadowCounter)/float(numSamples);
    temp = mix(vec4(temp,1), vec4(0, 0, 0, 1), shadow).xyz;
    return temp;
}

void main(){
    vec4 base_color = texture(texture0, fragTexCoord) * colDiffuse;
    vec3 cM = base_color.xyz; vec3 N = fragNormal;
    vec3 V = normalize(camPos - fragPosition);
    vec3 out_col = cM * ka;
    for (int i = 0; i < MAX_LIGHTS; i++){
        if (lights[i].enabled == 1){
            out_col += calculate_light(lights[i],light_vp[i],cM,N,V);
        }
    }
    finalColor = vec4(out_col.x,out_col.y,out_col.z,base_color.w);
    finalColor = pow(finalColor, vec4(1.0/2.2));
}

)";

const char* skybox_fs = R"(

#version 330

in vec3 fragPosition;
uniform samplerCube environmentMap;
uniform vec4 colDiffuse;
uniform int colorBins;
out vec4 finalColor;

void main(){
    vec3 color = vec3(0.0);
    color = texture(environmentMap, fragPosition).rgb;
    finalColor = vec4(color, 1.0) * colDiffuse;finalColor[3] = 1.0;
}

)";

const char* skybox_vs = R"(

#version 330

in vec3 vertexPosition;
uniform mat4 matProjection;
uniform mat4 matView;
out vec3 fragPosition;

void main(){
    fragPosition = vertexPosition;
    mat4 rotView = mat4(mat3(matView));
    vec4 clipPos = matProjection*rotView*vec4(vertexPosition, 1.0);
    gl_Position = clipPos;
}

)";

const char* lights_vs = R"(

#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in mat4 instanceTransform;

uniform mat4 mvp;
uniform mat4 matNormal;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

void main(){
    fragPosition = vec3(instanceTransform*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    mat4 rot = instanceTransform;
    rot[3] = vec4(0,0,0,1);
    fragNormal = normalize(vec3(rot*matNormal*vec4(vertexNormal, 1.0)));
    gl_Position = mvp*instanceTransform*vec4(vertexPosition, 1.0);
}

)";