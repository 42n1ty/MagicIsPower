#pragma once

#include <unordered_map>
#include <functional>

#include "components.hpp"
#include "../graphics/i_renderer.hpp"


namespace game {
  
  enum class CastType {
    Continuous,
    Persistent
  };
  
  struct SkillConf {
    std::string name;
    uint32_t tagsMask;
    CastType castType;
    
    float baseDmg;
    float baseRadius;
    float baseCd;
    uint8_t baseProj;
    
    std::function<ecs::EntID(ecs::Manager&, glm::vec2, glm::vec2, const ActiveSkillGem&)> buildPrefub;
  };
  
  struct SupConf {
    std::string name;
    std::function<void(ActiveSkillGem&, int)> applyMods;
  };
  
  class SkillDB {
    
    mip::IRenderer* m_rend{nullptr};
    std::unordered_map<std::string, std::shared_ptr<mip::IMaterial>> m_mats;
    
    std::shared_ptr<mip::IMaterial> getMaterial(const std::string& path) {
      if(m_mats.count(path) > 0) return m_mats[path];
      
      auto mat = m_rend->createMaterial("../../assets/shaders/shader.spv");
      if(auto tex = m_rend->createTexture(path, false)) {
        mat->setTexture(0, tex);
      }
      m_mats[path] = mat;
      
      return mat;
    }
    
  public:
    
    std::unordered_map<uint32_t, SkillConf> activeSkills;
    std::unordered_map<uint32_t, SupConf> supSkills;
    
    SkillDB(mip::IRenderer* rend) 
      : m_rend(rend) {
      
      //1. Active skills
      activeSkills[Hash("fireball")] = {
        .name = "Fireball",
        .tagsMask = SkillTag::Fire | SkillTag::Projectile,
        .castType = CastType::Continuous,
        .baseDmg = 25.f,
        .baseRadius = 15.f,
        .baseCd = 0.8f,
        .baseProj = 1,
        .buildPrefub = [this](ecs::Manager& manager, glm::vec2 pos, glm::vec2 vel, const ActiveSkillGem& gem) {
          ecs::EntID e = manager.createEntity();
          float timer = 3.f;
          
          manager.addComponent(e, Active{});
          manager.addComponent(e, WeaponTag{});
          manager.addComponent(e, Kinematics{ .z = 15, .pos = pos, .scale = {gem.finalRadius * 2, gem.finalRadius * 2}, .vel = vel });
          manager.addComponent(e, CircleCollider{.radius = gem.finalRadius});
          manager.addComponent(e, DamageDealer{.amount = gem.finalDmg, .dmgType = SkillTag::Fire});
          manager.addComponent(e, Lifetime{.curTimer = timer, .maxTimer = timer});
          manager.addComponent(e, Sprite{.mesh = m_rend->getGlobalQuad(), .material = getMaterial("../../assets/textures/fb.png")});
          
          return e;
        }
      };
      
      activeSkills[Hash("aura")] = {
        .name = "Nuclear",
        .tagsMask = SkillTag::Aura | SkillTag::AoE | SkillTag::Fire,
        .castType = CastType::Persistent,
        .baseDmg = 5.f,
        .baseRadius = 150.f,
        .baseCd = 0.5f,
        .baseProj = 0,
        .buildPrefub = [this, rend](ecs::Manager& manager, glm::vec2 pos, glm::vec2 vel, const ActiveSkillGem& gem) {
          ecs::EntID e = manager.createEntity();
          
          manager.addComponent(e, Active{});
          manager.addComponent(e, WeaponTag{});
          manager.addComponent(e, Kinematics{ .z = 8, .pos = pos, .scale = {gem.finalRadius * 2, gem.finalRadius * 2}, .vel = vel });
          manager.addComponent(e, CircleCollider{.radius = gem.finalRadius});
          manager.addComponent(e, DamageDealer{.amount = gem.finalDmg, .dmgType = SkillTag::Fire});
          manager.addComponent(e, PulseCooldown{.curTimer = gem.finalCd, .maxTimer = gem.finalCd});
          manager.addComponent(e, Sprite{.mesh = m_rend->getGlobalQuad(), .material = getMaterial("../../assets/textures/222.png")});
          
          return e;
        }
      };
      
      //2. Supports
      supSkills[Hash("added_fire")] = {
        "Added fire damage",
        [](ActiveSkillGem& targetGem, int supLvl) {
          targetGem.finalDmg += 10.f * supLvl;
          targetGem.tagsMask |= SkillTag::Fire;
        }
      };
    }
  };
  
}; //game