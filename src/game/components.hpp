#pragma once

#include <glm/glm.hpp>
#include "../common/ecs_core.hpp"
#include "../common/task.hpp"
#include "../graphics/i_texture.hpp"
#include "../graphics/i_mesh.hpp"
#include "../graphics/i_material.hpp"


namespace game {
  
  struct Kinematics {
    glm::vec2 pos{0.f, 0.f}; //4+4
    glm::vec2 scale{10.f, 10.f}; //pixels //4+4
    glm::vec2 vel = {0.f, 0.f}; //4+4
    float rot = 0.f; //degrees //4
    float speed; //4
    uint32_t z = 0; //4
  }; //36
  
  struct Sprite {
    // ecs::Handle<std::shared_ptr<mip::ITexture>> texHandle;
    std::shared_ptr<mip::IMesh> mesh; //16
    std::shared_ptr<mip::IMaterial> material; //16
    glm::vec4 uvRect{0.f, 0.f, 1.f, 1.f}; //4x4
  }; //48
  
  struct ColorTint {
    glm::vec4 baseColor{1.f, 1.f, 1.f, 1.f}; //4x4
    glm::vec4 curColor = baseColor; //4x4
  }; //32
  
  struct FlashEffect {
    float maxTime = 0.3f;
    float curTime = 0.f;
    glm::vec4 color = {1.f, 0.f, 0.f, 1.f}; // e.g. red - damage, green - poison, blue - freeze
  }; //24
  
  
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
  }; //32
  
  struct CircleCollider {
    float radius;
  }; //4
  
  struct Script {
    Task task;
    float timer = 0.f;
    bool active = true;
  }; //16
  
  struct BgTile {
    glm::vec2 offset;
  }; //8
  
  struct PlayerTag {};
  struct EnemyTag {};
  struct WeaponTag {};
  struct Active { bool value = true; };
  
  struct UITag {};
  enum class BarType{HP, EXP};
  struct UIProgressBar {
    BarType bType;
    float maxW{100.f}; //%
  };
  enum class AnchorH {Left, Center, Right, Stretch};
  enum class AnchorV {Top, Center, Bottom, Stretch};
  struct UIAnchor {
    AnchorH hAlign = AnchorH::Center;
    AnchorV vAlign = AnchorV::Center;
    glm::vec2 padding{0.f, 0.f};
    glm::vec2 baseSize{100.f, 8.f};
  };
  
  struct Exp {
    float cur = 0.f;
    float max = 10.f;
    uint32_t curLvl = 0;
    uint32_t maxLvl = 10;
  }; //16
  struct GameState {
    bool isPaused = false;
    bool isLvlUp = false;
  }; //8
  
  struct Health {
    float cur;
    float max;
    float iFrames = 0.f;
  }; //12
  
  struct DamageDealer {
    float amount;
  };
  
  struct PulseCooldown {
    float curTimer = 1.f;
    float maxTimer = 1.f;
  }; //8
  
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
    glm::vec2 offset{0.f, 0.f};
  }; //12
  
  struct Pierce {
    int count = 1;
  };
}; //game