#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D shadowMap;


void main()
{             
    vec3 projCoords = gl_FragCoord.xyz / gl_FragCoord.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    FragColor = vec4(vec3(closestDepth), 1.0); // orthographic
}