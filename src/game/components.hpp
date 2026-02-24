#pragma once

#include <glm/glm.hpp>
#include "../common/ecs_core.hpp"
#include "../common/task.hpp"
#include "../graphics/i_texture.hpp"
#include "../graphics/i_mesh.hpp"
#include "../graphics/i_material.hpp"


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
  
  struct Velocity {
    glm::vec2 value = {0.f, 0.f};
  };
  
  struct CircleCollider {
    float radius;
  };
  
  struct Script {
    Task task;
    float timer = 0.f;
    bool active = true;
  };
  
  struct BgTile {
    glm::vec2 offset;
  };
  
  
  struct PlayerTag {};
  struct EnemyTag {};
  struct Active { bool value = true; };
  
  
  struct Health {
    float cur;
    float max;
    float iFrames = 0.f;
  };
  
  struct DamageDealer {
    float amount;
  };
  
  struct Lifetime {
    float timer;
  };
  
  struct AttachTo {
    ecs::EntID target;
  };
  
  struct Pierce {
    int count = 1;
  };
}; //game