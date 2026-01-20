#version 450 core // old shader w/ light but I'm lazy to do it right now
out vec4 fragColor;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;

struct Material {
  sampler2D texture_diffuse1;
  sampler2D texture_specular1;
  float shininess;
  bool hasDiffTex;
};
uniform Material material;

uniform vec3 objClr;

struct DirLight {
  vec3 dir;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};
uniform DirLight dirLight;

struct SpotLight {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  
  vec3 pos;
  vec3 dir;
  float cutoff;
  float outerCutoff;
  
  float constant;
  float linear;
  float quadratic;
};
uniform SpotLight spotLight;

uniform vec3 viewPos;


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffColor, float specStrength) {
  vec3 lDir = normalize(-light.dir);
  
  float diff = max(dot(normal, lDir), 0.f);
  
  vec3 reflectDir = reflect(-lDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.f), material.shininess);
  
  vec3 ambient = light.ambient * diffColor;
  vec3 diffuse = light.diffuse * diff * diffColor;
  vec3 specular = light.specular * spec * specStrength;
  
  return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPosition, vec3 viewDir, vec3 diffColor, float specStrength) {
  vec3 lDir = normalize(light.pos - fragPosition);
  
  float diff = max(dot(normal, lDir), 0.f);
  
  vec3 reflectDir = reflect(-lDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.f), material.shininess);
  
  float distance = length(light.pos - fragPosition);
  float attenuation = 1.f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
  float theta = dot(lDir, normalize(-light.dir));
  float epsilon = light.cutoff - light.outerCutoff;
  float intencity = clamp((theta - light.outerCutoff) / epsilon, 0.f, 1.f);
  
  vec3 ambient = light.ambient * diffColor;
  vec3 diffuse = light.diffuse * diff * diffColor;
  vec3 specular = light.specular * spec * specStrength;
  
  return (ambient + diffuse + specular) * attenuation * intencity;
}


void main() {
    vec3 diffuseClr = objClr;
    
    if(material.hasDiffTex) {
      diffuseClr = texture(material.texture_diffuse1, texCoord).rgb;
    }
    
    // vec3 norm = normalize(normal);
    // vec3 viewDir = normalize(viewPos - fragPos);
    
    // float specStr = texture(material.texture_specular1, texCoord).r;
    
    vec3 result = diffuseClr;
    //vec3 result = CalcDirLight(dirLight, norm, viewDir, diffuseClr, specStr);
    //result += CalcSpotLight(spotLight, norm, fragPos, viewDir, diffuseClr, specStr);
    
    fragColor = vec4(result, 1.f);
    // fragColor = vec4(1.f, 0.f, 0.f, 1.f);
    // fragColor = vec4(texCoord.x, texCoord.y, 0.0, 1.f);
    
}