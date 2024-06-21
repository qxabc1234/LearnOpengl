#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightDir;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
} fs_in;
  
uniform vec3 lightDir; 
uniform vec3 viewPos; 
uniform vec3 lightColor;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_specular1;
uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, float bias)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
   //float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;
   if(projCoords.z > 1.0)
        shadow = 0.0;
    return shadow;
}

void main()
{        
    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor * vec3(texture(texture_diffuse1, fs_in.TexCoords));
  	
    // diffuse 
    vec3 normal = texture(texture_normal1, fs_in.TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0); 
    vec3 color = texture(texture_diffuse1, fs_in.TexCoords).rgb;
    vec3 lightDir = normalize(fs_in.TangentLightDir);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor * color;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor * vec3(texture(texture_specular1, fs_in.TexCoords));
     
        // calculate shadow
    float bias = 0.005;// max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, bias);                      
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);    
    
    FragColor = vec4( lighting ,1.0);

}