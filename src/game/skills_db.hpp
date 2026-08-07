#pragma once

#include <unordered_map>
#include <functional>

#include "data.hpp"
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
    
    DmgPart baseDmgParts[7];
    uint8_t baseDmgCnt = 0;
    
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
    ecs::Handle<std::shared_ptr<mip::IMesh>> m_globalQuad;
    std::unordered_map<std::string, ecs::Handle<std::shared_ptr<mip::IMaterial>>> m_mats;
    
    ecs::Handle<std::shared_ptr<mip::IMaterial>> getMaterial(const std::string& path, ecs::Manager& manager, const std::string& key) {
      if(m_mats.count(path) > 0) return m_mats[path];
      
      auto mat = m_rend->createMaterial("../../assets/shaders/shader.spv");
      auto tex = manager.loadAsset<std::shared_ptr<mip::ITexture>>(path);
      if(auto t = manager.getAsset(tex)) {
        mat->setTexture(0, *t);
      }
      auto finalMat = manager.insertAsset("mat." + key, std::move(mat));
      m_mats[path] = finalMat;
      
      return finalMat;
    }
    
  public:
    
    std::unordered_map<uint32_t, SkillConf> activeSkills;
    std::unordered_map<uint32_t, SupConf> supSkills;
    
    SkillDB(mip::IRenderer* rend, ecs::Handle<std::shared_ptr<mip::IMesh>> globalQuad) 
      : m_rend(rend), m_globalQuad(globalQuad) {
      
      //1. Active skills
      activeSkills[Hash("fireball")] = {
        .name = "Fireball",
        .tagsMask = SkillTag::Projectile,
        .castType = CastType::Continuous,
        .baseDmgParts = {{.type = DmgType::Fire, .amount = 10.f, .pen = 0}},
        .baseDmgCnt = 1,
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
          auto& dd = manager.addComponent(e, DamageDealer{});
          dd.count = gem.dmgCnt;
          for(int i = 0; i < gem.dmgCnt; ++i) dd.parts[i] = gem.finalDmgParts[i];
          manager.addComponent(e, Lifetime{.curTimer = timer, .maxTimer = timer});
          manager.addComponent(e, Pierce{.count = 2});
          manager.addComponent(e, Sprite{.mesh = m_globalQuad, .material = getMaterial("../../assets/textures/fb.png", manager, "fireball")});
          // manager.addComponent(e, ColorTint{.baseColor = {1.f, 0.2f, 0.f, 1.f}});
          
          return e;
        }
      };
      
      activeSkills[Hash("aura")] = {
        .name = "Nuclear",
        .tagsMask = SkillTag::Aura | SkillTag::AoE,
        .castType = CastType::Persistent,
        .baseDmgParts = {{.type = DmgType::Fire, .amount = 5.f, .pen = 0}},
        .baseDmgCnt = 1,
        .baseRadius = 150.f,
        .baseCd = 0.5f,
        .baseProj = 0,
        .buildPrefub = [this, rend](ecs::Manager& manager, glm::vec2 pos, glm::vec2 vel, const ActiveSkillGem& gem) {
          ecs::EntID e = manager.createEntity();
          
          manager.addComponent(e, Active{});
          manager.addComponent(e, WeaponTag{});
          manager.addComponent(e, Kinematics{ .z = 1, .pos = pos, .scale = {gem.finalRadius * 2, gem.finalRadius * 2}, .vel = vel });
          manager.addComponent(e, CircleCollider{.radius = gem.finalRadius});
          auto& dd = manager.addComponent(e, DamageDealer{});
          dd.count = gem.dmgCnt;
          for(int i = 0; i < gem.dmgCnt; ++i) dd.parts[i] = gem.finalDmgParts[i];
          manager.addComponent(e, PulseCooldown{.curTimer = gem.finalCd, .maxTimer = gem.finalCd});
          manager.addComponent(e, Sprite{.mesh = m_globalQuad, .material = getMaterial("../../assets/textures/222.png", manager, "garlic")});
          
          return e;
        }
      };
      
      //2. Supports
      supSkills[Hash("added_fire")] = {
        "Added fire damage",
        [](ActiveSkillGem& targetGem, int supLvl) {
          targetGem.finalDmgParts[DmgType::Fire].amount += 10.f * supLvl;
        }
      };
    }
  };
  
}; //game