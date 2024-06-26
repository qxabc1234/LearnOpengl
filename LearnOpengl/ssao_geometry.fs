#version 330 core
layout (location = 0) out vec4 gPositionDepth;

in vec2 TexCoords;
in vec3 FragPos;

const float NEAR = 0.1; // Projection matrix's near plane distance
const float FAR = 100.0f; // Projection matrix's far plane distance
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));	
}

void main()
{    
    // Store the fragment position vector in the first gbuffer texture
    gPositionDepth.xyz = FragPos;
    // And store linear depth into gPositionDepth's alpha component
    gPositionDepth.a = LinearizeDepth(gl_FragCoord.z); // Divide by FAR if you need to store depth in range 0.0 - 1.0 (if not using floating point colorbuffer)
}