#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "../common/ecs_core.hpp"
#include "components.hpp"
#include "../graphics/i_renderer.hpp"
#include "../graphics/i_material.hpp"


namespace game {
  
  class VisualEffectsSystem : public ecs::ISystem {
  public:
    void update(ecs::Manager& manager, const float dT) override {
      auto& clrs = manager.view<game::ColorTint>();
      auto& flashes = manager.view<game::FlashEffect>();
      
      const auto& flashOwners = flashes.getOwners();
      auto& flashDense = flashes.getDense();
      
      if(flashDense.size() > 0) {
        for(int i = static_cast<int>(flashDense.size()) - 1; i >= 0; --i) {
          auto& flash = flashDense[i];
          ecs::EntID e = flashOwners[i];
          
          auto* clr = clrs.get(e);
          
          if(flash.curTime > 0.f) {
            flash.curTime -= dT;
            
            float t = std::max(flash.curTime / flash.maxTime, 0.f);
            
            clr->curColor = glm::mix(clr->baseColor, flash.color, t);
            
            if(flash.curTime <= 0.f) {
              manager.removeComponent<game::FlashEffect>(e);
              clr->curColor = clr->baseColor;
            }
          }
          else {
            if(clr) clr->curColor = clr->baseColor;
          }
        }
      }
    }
  };
  
  class RenderSystem : public ecs::ISystem {
    
    struct RenderItem {
      ecs::EntID e;
      uint32_t z;
    };
    
    mip::IRenderer* renderer;
    std::vector<RenderItem> rendQ;
    
  public:
    RenderSystem(mip::IRenderer* rend) : renderer(rend) {
      rendQ.reserve(100);
    }
    
    void update(ecs::Manager& manager, const float dT) override {
      rendQ.clear();
      auto& sprites = manager.view<Sprite>();
      auto& ks = manager.view<Kinematics>();
      auto& clrs = manager.view<ColorTint>();
      
      for(ecs::EntID entity : sprites.getOwners()) {
        Kinematics* k = ks.get(entity);
        if(!k) continue;
        
        rendQ.push_back({entity, k->z});
      }
      
      std::sort(rendQ.begin(), rendQ.end(), [](const auto& a, const auto& b) {
        return a.z < b.z;
      });
      
      for(const auto& item : rendQ) {
        auto* active = manager.getComponent<Active>(item.e);
        if(active && !active->value) continue;
        auto* spr = sprites.get(item.e);
        auto* k = ks.get(item.e);
        auto* clr = clrs.get(item.e);
        
        glm::mat4 model = glm::mat4(1.f);
        model = glm::translate(model, glm::vec3(k->pos, 0.f));
        model = glm::rotate(model, glm::radians(k->rot), glm::vec3(0.f, 0.f, 1.f));
        model = glm::scale(model, glm::vec3(k->scale, 1.f));
        
        mip::RenderInfo info{
          .transform = model,
          .uvRect = spr->uvRect,
          .color = clr ? clr->curColor : glm::vec4{1.f, 1.f, 1.f, 1.f}
        };
        
        renderer->submit(spr->mesh, spr->material, info);
      }
      
    }
  };
  
  class AnimSystem : public ecs::ISystem {
  public:
    void update(ecs::Manager& manager, const float dT) override {
      auto& animators = manager.view<game::Animator>();
      auto& sprites = manager.view<game::Sprite>();
      auto& acts = manager.view<game::Active>();
      
      for(auto ae : animators.getOwners()) {
        auto* act = acts.get(ae);
        if(act && !act->value) continue;
        
        auto* anim = animators.get(ae);
        auto* spr = sprites.get(ae);
        
        if(!spr) continue;
        
        anim->timer += dT;
        
        if(anim->timer >= anim->frameTime) {
          anim->timer -= anim->frameTime;
          anim->curFrame++;
          
          if(anim->curFrame >= anim->frameCnt) {
            if(anim->loop) anim->curFrame = 0;
            else anim->curFrame = anim->frameCnt - 1;
          }
        }
        
        int actFrame = anim->startFrame + anim->curFrame;
        
        float frameW = 1.f / anim->cols;
        float frameH = 1.f / anim->rows;
        
        float uvX = (actFrame % anim->cols) * frameW;
        float uvY = (actFrame % anim->rows) * frameH;
        
        spr->uvRect = {uvX, uvY, frameW, frameH};
      }
    }
  };
  
  class MovementSystem : public ecs::ISystem {
  public:
    MovementSystem() {}
    
    void update(ecs::Manager& manager, const float dT) override {
      auto& ks = manager.view<Kinematics>();
      
      const auto& kinOwners = ks.getOwners();
      auto& kinDense = ks.getDense();
      
      for(size_t i = 0; i < kinDense.size(); ++i) {
        auto* active = manager.getComponent<game::Active>(i);
        if(active && !active->value) continue;
        
        auto& k = kinDense[i];
        k.pos += k.vel * dT;
      }
      
    }
  };
  
  class PlayerControllerSystem : public ecs::ISystem {
    GLFWwindow* m_wnd;
    
  public:
    PlayerControllerSystem(GLFWwindow* wnd) : m_wnd(wnd) {}
    
    void update(ecs::Manager& manager, const float dT) override{
      auto& ks = manager.view<Kinematics>();
      auto& ps = manager.view<PlayerTag>();
      
      for(ecs::EntID e : ps.getOwners()) {
        auto* kin = ks.get(e);
        if(!kin) continue;
        
        glm::vec2 moveDir{0.f, 0.f};
        
        if (glfwGetKey(m_wnd, GLFW_KEY_W) == GLFW_PRESS) moveDir.y -= 1.f;
        if (glfwGetKey(m_wnd, GLFW_KEY_S) == GLFW_PRESS) moveDir.y += 1.f;
        if (glfwGetKey(m_wnd, GLFW_KEY_A) == GLFW_PRESS) moveDir.x -= 1.f;
        if (glfwGetKey(m_wnd, GLFW_KEY_D) == GLFW_PRESS) moveDir.x += 1.f;
        
        if(glm::length(moveDir) > 0.f) {
          moveDir = glm::normalize(moveDir);
        }
        
        kin->vel = moveDir * kin->speed;
      }
    }
  };
  
  class PatrolSystem : public ecs::ISystem {
  public:
    void update(ecs::Manager& manager, const float dT) override {
      auto& scripts = manager.view<Script>();
      
      for(ecs::EntID e : scripts.getOwners()) {
        auto* active = manager.getComponent<game::Active>(e);
        if(active && !active->value) continue;
        auto* scr = scripts.get(e);
        if(!scr->active || !scr->task.handle) continue;
        
        scr->timer -= dT;
        
        if(scr->timer <= 0.f) {
          bool running = scr->task.resume();
          
          if(running) {
            scr->timer = scr->task.handle.promise().waitTime;
          }
          else {
            scr->active = false;
          }
        }
      }
    }
  };
  
  class TileSystem : public ecs::ISystem {
  public:
    static constexpr float tileSize = 2'500.f;
    
    void update(ecs::Manager& manager, const float dT) override {
      glm::vec2 playerPos{0.f};
      for(auto& e : manager.view<game::PlayerTag>().getOwners()) {
        playerPos = manager.getComponent<game::Kinematics>(e)->pos;
        break;
      }
      
      auto& tiles = manager.view<game::BgTile>();
      auto& ks = manager.view<game::Kinematics>();
      for(auto& e : tiles.getOwners()) {
        auto* tile = tiles.get(e);
        auto* kin = ks.get(e);
        float targetX = std::round(playerPos.x / tileSize) * tileSize + tile->offset.x * tileSize;
        float targetY = std::round(playerPos.y / tileSize) * tileSize + tile->offset.y * tileSize;
        
        kin->pos = {targetX, targetY};
      }
    }
  };
  
  class EnemySpawnerSystem : public ecs::ISystem {
    mip::IRenderer* m_rend;
    std::vector<ecs::EntID> m_pool;
    Task m_waveTask{nullptr};
    float m_waveTimer = 0.f;
    float m_spawnRadius = 800.f;
    std::shared_ptr<mip::IMaterial> m_enemyMat;
    
    float m_speed; //temp need enemyConfigs later
    
  public:
    
    inline static uint32_t diedCount = 0;
    
    EnemySpawnerSystem(mip::IRenderer* rend) : m_rend(rend) {
      m_enemyMat = m_rend->createMaterial("../../assets/shaders/shader.spv");
      auto tex = m_rend->createTexture("../../assets/textures/mob1.png", false);
      m_enemyMat->setTexture(0, tex);
    }
    
    ecs::EntID createEnemy(ecs::Manager& manager, glm::vec2 spawnPos) {
      ecs::EntID e;
      if(!m_pool.empty()) {
        e = m_pool.back();
        m_pool.pop_back();
        manager.getComponent<game::Active>(e)->value = true;
        auto* kin = manager.getComponent<game::Kinematics>(e);
        kin->pos = spawnPos;
        kin->speed = m_speed;
        manager.getComponent<game::Health>(e)->cur = manager.getComponent<game::Health>(e)->max;
        // manager.getComponent<game::Health>(e)->iFrames = 0.5f;
      }
      else {
        e = manager.createEntity();
      
        manager.addComponent(e, game::EnemyTag{});
        manager.addComponent(e, game::Active{});
        manager.addComponent(e, game::Kinematics{
          .pos = {spawnPos.x, spawnPos.y},
          .scale = {50.f, 50.f},
          .rot = 0.f,
          .speed = m_speed,
          .z = 9
        });
        manager.addComponent(e, game::CircleCollider{.radius = manager.getComponent<Kinematics>(e)->scale.x / 2.f});
        manager.addComponent(e, game::Health{
          .max = 30.f,
          .iFrames = 0.5f
        });
        manager.getComponent<game::Health>(e)->cur = manager.getComponent<game::Health>(e)->max;
        manager.addComponent(e, game::Sprite{
          .mesh = m_rend->getGlobalQuad(),
          .material = m_enemyMat
        });
        manager.addComponent(e, game::ColorTint{});
        
        Logger::debug("Enemy was created: {}", e);
      }
      
      return e;
    }
    
    Task wave(ecs::Manager& manager) {
      float speed = 100.f;
      while(true) {
        spawnCircle(manager, 5, speed);
        co_yield 10.f;
        
        spawnCircle(manager, 10, speed + 50.f);
        co_yield 10.f;
        
        spawnCircle(manager, 15, speed + 100.f);
        co_yield 10;
      }
    }
    
    void spawnCircle(ecs::Manager& manager, int count, float speed) {
      m_speed = speed;
      glm::vec2 plPos{0.f, 0.f};
      for(auto e : manager.view<game::PlayerTag>().getOwners()) {
        plPos = manager.getComponent<game::Kinematics>(e)->pos;
        break;
      }
      
      for(int i = 0; i < count; ++i) {
        float angle = (360.f / count) * i;
        float rad = glm::radians(angle);
        
        glm::vec2 spawnPos = plPos + glm::vec2(cos(rad), sin(rad)) * m_spawnRadius;
        
        ecs::EntID e = createEnemy(manager, spawnPos);
        
        // auto* vel = manager.getComponent<game::Velocity>(e);
        // vel->value = glm::normalize(plPos - spawnPos) * m_speed;
      }
    }
    
    void update(ecs::Manager& manager, const float dT) override {
      if(!m_waveTask.handle) {
        m_waveTask = wave(manager);
      }
      
      m_waveTimer -= dT;
      if(m_waveTimer <= 0.f) {
        if(m_waveTask.resume()) {
          m_waveTimer = m_waveTask.handle.promise().waitTime;
        }
      }
      
      // move vectors
      glm::vec2 plPos{0.f, 0.f};
      for(auto e : manager.view<game::PlayerTag>().getOwners()) {
        plPos = manager.getComponent<game::Kinematics>(e)->pos;
        break;
      }
      auto& es = manager.view<game::EnemyTag>();
      for(auto e : es.getOwners()) {
        auto* act = manager.getComponent<game::Active>(e);
        if(act && !act->value) continue;
        auto* kin = manager.getComponent<game::Kinematics>(e);
        kin->vel = glm::normalize(plPos - kin->pos) * kin->speed;
      }
      
      // check HP
      auto& healths = manager.view<game::Health>();
      for (auto e : manager.view<game::EnemyTag>().getOwners()) {
        auto* active = manager.getComponent<game::Active>(e);
        if (active->value && healths.get(e)->cur <= 0) {
          active->value = false;
          Logger::debug("Enemy died! #{}", diedCount++);
          m_pool.push_back(e);
          // exp
        }
      }
    }
    
  };
  
  class DamageSystem : public ecs::ISystem {
    
    std::vector<ecs::EntID> m_weaponsToDestroy;
    
  public:
    void update(ecs::Manager& manager, const float dT) override {
      m_weaponsToDestroy.clear();
      
      auto& healths = manager.view<game::Health>();
      auto& enemies = manager.view<game::EnemyTag>();
      auto& circles = manager.view<game::CircleCollider>();
      auto& ks = manager.view<game::Kinematics>();
      auto& dds = manager.view<game::DamageDealer>();
      auto& acts = manager.view<game::Active>();
      auto& pulses = manager.view<game::PulseCooldown>();
      auto& players = manager.view<game::PlayerTag>();
      
      
      // 1st iter by weapons
      for(auto we : dds.getOwners()) {
        auto* wact = acts.get(we);
        if(wact && !wact->value) continue;
        auto* pulse = pulses.get(we);
        if(pulse && pulse->curTimer > 0.f) continue;
        auto* wc = circles.get(we);
        auto* wt = ks.get(we);
        if(!wc || !wt) continue;
        auto* dmg = dds.get(we);
        
        // 2nd iter by enemies
        for(auto ee : enemies.getOwners()) {
          auto* eact = acts.get(ee);
          if(eact && !eact->value) continue;
          auto* ehp = healths.get(ee);
          auto* ec = circles.get(ee);
          auto* et = ks.get(ee);
          
          if(!ec || !et) continue;
        
          float dist = glm::distance(wt->pos, et->pos);
          if(dist < (wc->radius + ec->radius)) {
            if(pulse) {
              if(pulse->curTimer <= 0.f) {
                ehp->cur -= dmg->amount;
              }
            }
            else ehp->cur -= dmg->amount; // once damage
            // flash effect after gaining damage
            float time = 0.2f;
            manager.addComponent(ee, game::FlashEffect{ .maxTime = time, .curTime = time, .color = {1.f, 0.f, 0.f, 1.f} });
            
            if(auto* applies = manager.getComponent<game::AppliesDoT>(we)) {
              auto* statuses = manager.getComponent<game::StatusEffects>(ee);
              if(!statuses) {
                statuses = &manager.addComponent(ee, game::StatusEffects{});
              }
              statuses->dots.emplace_back(game::DoTCharge{
                .damage = applies->dmgPerTick,
                .tickRate = applies->tickRate,
                .curTickTimer = applies->tickRate,
                .lifetime = applies->duration
              });
            }
            
            if(auto* pierce = manager.getComponent<game::Pierce>(we)) {
              pierce->count--;
              if(pierce->count <= 0) {
                // wact->value = false;
                m_weaponsToDestroy.emplace_back(we);
                break;
              }
            }
          }
        }
        
        if(pulse) pulse->curTimer = pulse->maxTimer;
      }
      
      //player & enemy colls
      for(auto pe : players.getOwners()) {
        auto* ph = healths.get(pe);
        auto* pt = ks.get(pe);
        auto* pc = circles.get(pe);
        for(auto ee : enemies.getOwners()) {
          auto* eact = acts.get(ee);
          if(eact && !eact->value) continue;
          auto* ehp = healths.get(ee);
          auto* ec = circles.get(ee);
          auto* et = ks.get(ee);
          
          if(!ec || !et) continue;
        
          float dist = glm::distance(pt->pos, et->pos);
          if(dist < (pc->radius + ec->radius)) {
            ph->cur -= 5.f;
            if(auto* spr = manager.getComponent<game::Sprite>(pe)) {
              // flash effect after gaining damage
              float time = 0.3f;
              manager.addComponent(pe, game::FlashEffect{ .maxTime = time, .curTime = time, .color = {1.f, 0.f, 0.f, 1.f} });
            }
            // Logger::info("Get hit!");
            ehp->cur = 0;
          }
          
        }
        
        break; //one player
      }
      
      // cd dots
      for(auto e : pulses.getOwners()) {
        auto* act = acts.get(e);
        if(act && !act->value) continue;
        if(auto* pulse = pulses.get(e); pulse->curTimer > 0.f) pulse->curTimer -= dT;
      }
      
      // destroy projectiles
      for (auto we : m_weaponsToDestroy) {
        manager.destroyEntity(we);
      }
      
    }
    
  };
  
  class StatusSystem : public ecs::ISystem {
  public:
    void update(ecs::Manager& manager, const float dT) override {
      auto& healths = manager.view<game::Health>();
      auto& statuses = manager.view<game::StatusEffects>();
      auto& acts = manager.view<game::Active>();
      
      for(auto se : statuses.getOwners()) {
        auto* act = acts.get(se);
        if(act && !act->value) continue;
        
        auto* hp = healths.get(se);
        auto* status = statuses.get(se);
        
        if(!hp || !status) continue;
        
        for(int i = static_cast<int>(status->dots.size()) - 1; i >= 0; --i) {
          auto& dot = status->dots[i];
          dot.lifetime -= dT;
          dot.curTickTimer -= dT;
          
          if(dot.curTickTimer <= 0.f) {
            hp->cur -= dot.damage;
            dot.curTickTimer = dot.tickRate;
          }
          
          if(dot.lifetime <= 0.f) {
            status->dots.erase(status->dots.begin() + i);
          }
        }
      }
    }
  };
  
  class AttachmentSystem : public ecs::ISystem {
    void update(ecs::Manager& manager, const float dT) override {
      auto& acts = manager.view<game::Active>();
      for(auto e : manager.view<game::AttachTo>().getOwners()) {
        auto* act = acts.get(e);
        if(act && !act->value) continue;
        auto te = manager.getComponent<game::AttachTo>(e)->target;
        auto* ks = manager.getComponent<game::Kinematics>(te);
        auto* itsKs = manager.getComponent<game::Kinematics>(e);
        if(ks && itsKs) itsKs->pos = ks->pos;
      }
    }
  };
  
  class LifetimeSystem : public ecs::ISystem {
    void update(ecs::Manager& manager, const float dT) override {
      auto& acts = manager.view<game::Active>();
      for(auto e : manager.view<game::Lifetime>().getOwners()) {
        auto* act = acts.get(e);
        if(act && !act->value) continue;
        auto* te = manager.getComponent<game::Lifetime>(e);
        if(te && te->curTimer > 0) te->curTimer -= dT;
        if(te->curTimer <= 0) act->value = false;
      }
    }
  };
  
}; //game