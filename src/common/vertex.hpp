#pragma once

#include <glm/glm.hpp>



static constexpr int MAX_BONES_PER_VERTEX = 4;
static constexpr uint16_t MAX_BONES = 100;

namespace mip {
  
  struct Vertex2D {
    glm::vec2 pos;
    glm::vec2 texCoord;
  };
  
  struct Vertex3D {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    
    glm::ivec4 boneIDs{-1, -1, -1, -1};
    glm::vec4 weights{0.f, 0.f, 0.f, 0.f};
  };
  
}; //mip