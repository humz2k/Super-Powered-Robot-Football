/**
 * @file lights.vs
 **/

#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

in mat4 instanceTransform;

// Input uniform values
uniform mat4 mvp;
//uniform mat4 matModel;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

// NOTE: Add here your custom variables

void main()
{
    // Send vertex attributes to fragment shader
    fragPosition = vec3(instanceTransform*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    mat4 rot = instanceTransform; rot[3] = vec4(0,0,0,1); ;//rot[0][3] = 0; rot[1][3] = 0; rot[2][3] = 0; rot[3][3] = 1;
    fragNormal = normalize(vec3(rot*matNormal*vec4(vertexNormal, 1.0)));
    // Calculate final vertex position, note that we multiply mvp by instanceTransform
    gl_Position = mvp*instanceTransform*vec4(vertexPosition, 1.0);
    // Send vertex attributes to fragment shader
    //fragPosition = vec3(matModel*vec4(vertexPosition, 1.0));
    //fragTexCoord = vertexTexCoord;
    //fragColor = vertexColor;
    //fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));

    // Calculate final vertex position
    //gl_Position = mvp*vec4(vertexPosition, 1.0);
}