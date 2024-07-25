/**
 * @file skybox.fs
 **/

#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;

// Input uniform values
uniform samplerCube environmentMap;

uniform vec4 colDiffuse;

uniform int colorBins;

// Output fragment color
out vec4 finalColor;

void main()
{
    //bool vflipped = 0;
    //bool doGamma = 0;
    // Fetch color from texture map
    vec3 color = vec3(0.0);

    //if (vflipped) color = texture(environmentMap, vec3(fragPosition.x, -fragPosition.y, fragPosition.z)).rgb;
    //else
    color = texture(environmentMap, fragPosition).rgb;

    //if (doGamma)// Apply gamma correction
    //{
    //    color = color/(color + vec3(1.0));
     //   color = pow(color, vec3(1.0/2.2));
    //}

    // Calculate final fragment color
    finalColor = vec4(color, 1.0) * colDiffuse;

    finalColor[3] = 1.0;
}