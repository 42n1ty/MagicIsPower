#pragma once

#include <glm/glm.hpp>
#include "../common/ecs_core.hpp"
#include "../common/task.hpp"
#include "../graphics/i_texture.hpp"
#include "../graphics/i_mesh.hpp"
#include "../graphics/i_material.hpp"


namespace game {
  
  constexpr uint32_t Hash(const char* str, uint32_t hash = 2166136261u) {
    return *str ? Hash(str + 1, (hash ^ static_cast<uint32_t>(*str)) * 16777619u) : hash;
  }
  
  struct Kinematics {
    uint8_t z = 0; //1
    glm::vec2 pos{0.f, 0.f}; //4+4
    glm::vec2 scale{10.f, 10.f}; //pixels //4+4
    glm::vec2 vel = {0.f, 0.f}; //4+4
    float rot = 0.f; //degrees //4
    float speed; //4
  }; //33
  
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
    uint8_t cols = 1;
    uint8_t rows = 1;
    
    uint16_t startFrame = 0;
    uint16_t frameCnt = 1;
    uint16_t curFrame = 0;
    
    bool loop = true;
    
    float frameTime = 0.05f;
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
  }; //16
  
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
  enum SkillTag : uint32_t {
    None = 0,
    Fire       = 1 << 0,
    Water      = 1 << 1,
    Earth      = 1 << 2,
    Air        = 1 << 3,
    Cold       = 1 << 4,
    Lightning  = 1 << 5,
    Aura       = 1 << 6,
    AoE        = 1 << 7,
    Projectile = 1 << 8,
  };
  struct DirtyStatsTag {};
  struct InventoryItem {
    ecs::EntID owner = ecs::NULL_ENT;
    bool isEquipped = false;
  };
  struct PlayerStats {
    //TODO: fixed percentage in uint16_t instead of float to minimize size ???
    float incFireDmg = 0.f;
    float incColdDmg = 0.f;
    float incAoERadius = 0.f;
    float cdReduction = 0.f;
    uint32_t extraProj = 0;
  }; //20
  struct PermanentStats {
    float incFireDmg = 0.f;
    float incColdDmg = 0.f;
    float incAoERadius = 0.f;
    float cdReduction = 0.f;
    uint32_t extraProj = 0;
  }; //
  struct ActiveSkillGem {
    uint32_t skillIdHash;
    uint32_t tagsMask;
    int lvl = 1;
    
    float finalDmg = 0.f;
    float finalCd = 0.f;
    float finalRadius = 0.f;
    uint32_t finalProj = 0;
    
    float curCdTimer = 0.f;
    float dmgMultiplier = 1.f;
    float curLvlDmg = 0.f; //actually unused except for GamePlayUISystem
    
    ecs::EntID spawnedEnt = ecs::NULL_ENT;
  }; //44
  struct SupGem {
    uint32_t supIdHash;
    int lvl = 1;
  };
  struct LinkedGems {
    ecs::EntID gems[7] = {
      ecs::NULL_ENT,
      ecs::NULL_ENT,
      ecs::NULL_ENT,
      ecs::NULL_ENT,
      ecs::NULL_ENT,
      ecs::NULL_ENT,
      ecs::NULL_ENT
    };
    uint8_t max = 3;
    uint8_t cur = 0;
    
    bool add(ecs::EntID sup) {
      if(cur < max) {
        gems[cur++] = sup;
        return true;
      }
      return false;
    }
    bool remove(ecs::EntID sup) {
      if(cur > 0) {
        for(auto s : gems) {
          if(s == sup) {
            std::swap(s, gems[cur]);
            gems[cur] = ecs::NULL_ENT;
            return true;
          }
        }
      }
      return false;
    }
  }; //29
  
  
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
  }; //2+4=6
  
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
  
  struct AttachTo {
    ecs::EntID target;
    glm::vec2 offset{0.f, 0.f};
  }; //12
  
  struct DamageDealer {
    float amount;
    SkillTag dmgType;
    float pen = 0.f;
  }; //12
  struct Resistances {
    float fire = 0.f;
    float water = 0.f;
    float earth = 0.f;
    float air = 0.f;
    float cold = 0.f;
    float lightning = 0.f;
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
  
  struct Pierce {
    int count = 1;
  };
}; //game