#pragma once

#include <glm/glm.hpp>
#include "../common/ecs_core.hpp"
#include "../graphics/i_texture.hpp"
#include "../graphics/i_mesh.hpp"


namespace game {
  
  struct Transform {
    glm::vec2 pos{0.f, 0.f};
    glm::vec2 scale{10.f, 10.f}; //pixels
    float rot = 0.f; //degrees
    uint32_t z = 0;
  };
  
  struct Sprite {
    // ecs::Handle<std::shared_ptr<mip::ITexture>> texHandle;
    std::shared_ptr<mip::IMesh> mesh;
    std::shared_ptr<mip::IMaterial> material;
    glm::vec4 clr{1.f, 1.f, 1.f, 1.f};
  };
  
}; //game