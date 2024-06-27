#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform vec3 lightDir; 
uniform vec3 lightColor;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;


void main()
{             
    // Retrieve data from g-buffer
    vec3 FragPos = texture(gPositionDepth, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    float AmbientOcclusion = texture(ssao, TexCoords).r;
    float shadow = texture(gNormal, TexCoords).a;
    
    vec3 ambient = vec3(0.3 * AmbientOcclusion) * Diffuse; // <-- this is where we use ambient occlusion

    // diffuse 
    Normal = Normal * 2 - 1;
    float diff = max(dot(lightDir, Normal), 0.0);
    vec3 diffuse = diff * lightColor * Diffuse;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir  = normalize(-FragPos); // Viewpos is (0.0.0)
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * lightColor * spec * vec3(Specular);
                    
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);    
    
    FragColor = vec4( lighting ,1.0);
}