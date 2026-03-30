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
    glm::vec4 baseColor{1.f, 1.f, 1.f, 1.f};
    glm::vec4 curColor{1.f, 1.f, 1.f, 1.f};
    glm::vec4 uvRect{0.f, 0.f, 1.f, 1.f};
  };
  
  struct FlashEffect {
    float curTime = 0.f;
    float maxTime = 0.3f;
    glm::vec4 color = {1.f, 0.f, 0.f, 1.f}; // e.g. red - damage, green - poison, blue - freeze
  };
  
  
  struct Animator {
    // sprite-sheet grid
    int cols = 1;
    int rows = 1;
    
    // current anim
    int startFrame = 0;
    int frameCnt = 1;
    float frameTime = 0.05f;
    bool loop = true;
    
    // current state
    int curFrame = 0;
    float timer = 0.f;
    
    void play(int start, int count, float speed, bool isLoop = true) {
      if(startFrame == start && frameCnt == count) return;
      startFrame = start;
      frameCnt = count;
      frameTime = speed;
      loop = isLoop;
      curFrame = 0;
      timer = 0.f;
    }
  };
  
  struct Velocity {
    glm::vec2 value = {0.f, 0.f};
    float speed;
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
  
  struct PulseCooldown {
    float curTimer = 1.f;
    float maxTimer = 1.f;
  };
  
  struct DoTCharge {
    float damage;
    float tickRate;
    float curTickTimer;
    float lifetime;
  };
  
  struct StatusEffects {
    std::vector<DoTCharge> dots;
  };
  
  struct AppliesDoT {
    float dmgPerTick;
    float tickRate;
    float duration;
  };
  
  struct Lifetime {
    float curTimer;
    float maxTimer;
  };
  
  struct AttachTo {
    ecs::EntID target;
  };
  
  struct Pierce {
    int count = 1;
  };
}; //game