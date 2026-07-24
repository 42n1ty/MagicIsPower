#pragma once

#include <glm/glm.hpp>
#include "../common/ecs_core.hpp"
#include "../common/task.hpp"
#include "../graphics/i_texture.hpp"
#include "../graphics/i_mesh.hpp"
#include "../graphics/i_material.hpp"
#include "item_types.hpp"


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
  enum DmgType : uint8_t {
    Physical = 0, Fire, Air, Water, Earth, Cold, Lightning, Light, Darkness, Sonic, Mental, Astral, Spatial, Fate, Acid, Poison, Pure,
    Count
  };
  struct DmgPart {
    DmgType type;
    float amount;
    float pen;
  };
  struct DamageDealer {
    DmgPart parts[7];
    uint8_t count = 0;
  }; //64
  struct Resistances {
    float res[DmgType::Count] = {0.f};
  };
  enum SkillTag : uint32_t {
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
    float incDmg[DmgType::Count] = {0.f};
    float flatAddedDmg[DmgType::Count] = {0.f};
    float penetration[DmgType::Count] = {0.f};
    
    float incAoERadius = 1.f;
    float incAoEDmg = 0.f;
    float cdReduction = 0.f;
    uint32_t extraProj = 0;
  }; //
  struct PermanentStats {
    float incDmg[DmgType::Count] = {0.f};
    
    float incAoERadius = 1.f;
    float cdReduction = 0.f;
    uint32_t extraProj = 0;
  }; //
  struct ActiveSkillGem {
    uint32_t skillIdHash;
    uint32_t tagsMask;
    int lvl = 1;
    
    DmgPart finalDmgParts[7];
    uint8_t dmgCnt = 0;
    
    float finalCd = 0.f;
    float finalRadius = 0.f;
    uint32_t finalProj = 0;
    float dmgMultiplier = 1.f;
    
    float curCdTimer = 0.f;
    float curLvlDmg = 0.f; //actually unused except for GamePlayUISystem
    
    ecs::EntID spawnedEnt = ecs::NULL_ENT;
    
    void addFinalDmg(DmgType t, float amt, float p = 0.f) {
      for(int i = 0; i < dmgCnt; ++i) {
        if(finalDmgParts[i].type == t) {
          finalDmgParts[i].amount += amt;
          finalDmgParts[i].pen += p;
          return;
        }
      }
      if(dmgCnt < 7) {
        finalDmgParts[dmgCnt++] = {t, amt, p};
      }
    }
  }; //
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
  struct PlayerLootFilter {
    std::vector<LootFilterRule> rules;
    
    PlayerLootFilter() {
      rules.push_back({
        true,
        "Always show Gems & Relics",
        static_cast<uint32_t>(ItemRarity::Relic),
        static_cast<uint32_t>(ItemCategory::SkillGem),
        FilterAction::Show
      });
      rules.push_back({
        true,
        "Salvage Junk gear",
        static_cast<uint32_t>(ItemRarity::Junk),
        static_cast<uint32_t>(ItemCategory::Armor) | static_cast<uint32_t>(ItemCategory::Weapon),
        FilterAction::AutoSalvage
      });
      
      rules.push_back({true, "Default", 0xFFFFFFFF, 0xFFFFFFFF, FilterAction::Show});
    }
    
    FilterAction evaluate(ItemRarity rarity, ItemCategory category) {
      for(const auto& rule : rules) {
        if(!rule.enable) continue;
        bool matchRarity = (static_cast<uint32_t>(rarity) & rule.rarityMask) != 0;
        bool matchCategory = (static_cast<uint32_t>(category) & rule.categoryMask) != 0;
        
        if(matchCategory && matchRarity) {
          return rule.action;
        }
      }
      
      return FilterAction::Show;
    }
  };
  struct GroundItem {
    std::string name;
    ItemRarity rarity;
  };
  struct Materials {
    uint32_t scrapMetal = 0;
  };
  
  
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
    uint32_t cur = 0;
    uint32_t max = 10;
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