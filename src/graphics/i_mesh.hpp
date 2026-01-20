#pragma once

#include <vector>
#include "../common/vertex.hpp"

namespace mip {
  
  struct MeshData {
    std::vector<Vertex2D> vertices;
    std::vector<uint32_t> indices;
  };
  
  /**
   * @brief Mesh Interface
   * Handle for vertex/index gpu buffers for multi-api renderer
   */
  class IMesh {
  public:
    virtual ~IMesh() = default;
    
    static MeshData createQuad() {
      return {
        //vertices
        {
          {{-0.5f, -0.5f}, {0.f, 0.f}},
          {{0.5f, -0.5f},  {1.f, 0.f}},
          {{0.5f, 0.5f},   {1.f, 1.f}},
          {{-0.5f, 0.5f},  {0.f, 1.f}}
        },
        //indices
        {0, 1, 2, 2, 3, 0}
      };
    }
    
  };
  
}; //mip