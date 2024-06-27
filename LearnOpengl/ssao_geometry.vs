#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    vec4 viewPos = view * model * vec4(aPos, 1.0f);
    FragPos = viewPos.xyz; 
    gl_Position = projection * viewPos;
    TexCoords = aTexCoords;

    FragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0);
    
    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    Normal = normalMatrix * aNormal;

}