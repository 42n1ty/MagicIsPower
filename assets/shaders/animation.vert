#version 450 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in ivec4 aBoneIDs;
layout(location=4) in vec4 aWeights;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

const int MAX_BONES = 100;
uniform mat4 gBones[MAX_BONES];


void main() {
  mat4 boneTransform = mat4(0.f);
  float totalWeight = 0.0;
  
  for(int i = 0; i < 4; ++i) {
    if(aBoneIDs[i] == -1) {
      continue;
    }
    if(aBoneIDs[i] >= MAX_BONES) {
      boneTransform += mat4(1.f) * aWeights[i];
      totalWeight += aWeights[i];
      continue;
    }
    
    boneTransform += gBones[aBoneIDs[i]] * aWeights[i];
    totalWeight += aWeights[i];
  }
  if (totalWeight < 0.001) {
    boneTransform = mat4(1.0);
  }
  vec4 posL = boneTransform * vec4(aPos, 1.f);
  
  gl_Position = projection * view * model * posL;
  
  fragPos = vec3(model * posL);
  mat3 normalMatrix = mat3(transpose(inverse(model * boneTransform)));
  normal = normalMatrix * aNormal;
  
  texCoord = aTexCoord;
}