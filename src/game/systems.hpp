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
      auto& clrs = manager.view<ColorTint>();
      auto& flashes = manager.view<FlashEffect>();
      
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
              manager.removeComponent<FlashEffect>(e);
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
        
        glm::ivec4 opts{};
        opts.x = manager.getComponent<UITag>(item.e) ? 1 : 0;
        mip::RenderInfo info{
          .transform = model,
          .uvRect = spr->uvRect,
          .color = clr ? clr->curColor : glm::vec4{1.f, 1.f, 1.f, 1.f},
          .options = opts
        };
        
        renderer->submit(spr->mesh, spr->material, info);
      }
      
    }
  };
  
  class UISystem : public ecs::ISystem {
    GLFWwindow* m_wnd{nullptr};
    
  public:
    
    UISystem(GLFWwindow* wnd) : m_wnd(wnd) {}
    
    void update(ecs::Manager& manager, const float dT) override {
      int w, h;
      glfwGetWindowSize(m_wnd, &w, &h);
      float scrW = static_cast<float>(w);
      float scrH = static_cast<float>(h);
      
      auto& ks = manager.view<Kinematics>();
      auto& bars = manager.view<UIProgressBar>();
      auto& anchors = manager.view<UIAnchor>();
      
      // step 1: layout
      for(auto ae : anchors.getOwners()) {
        auto* kin = ks.get(ae);
        auto* anch = anchors.get(ae);
        if(!kin || ! anch) continue;
        
        float outW = anch->baseSize.x;
        float outH = anch->baseSize.y;
        float outX = 0.f;
        float outY = 0.f;
        
        // h align, 0.0 < mesh.x < 1.0
        switch(anch->hAlign) {
          case AnchorH::Stretch: {
            outW = scrW - anch->padding.x * 2.f;
            outX = anch->padding.x;
            break;
          }
          case AnchorH::Left: {
            outX = anch->padding.x;
            break;
          }
          case AnchorH::Right: {
            outX = scrW - outW - anch->padding.x;
            break;
          }
          case AnchorH::Center: {
            outW = (scrW - outW) / 2.f;
            break;
          }
          default: break;
        }
        
        // v align, -0.5 < mesh.y < 0.5
        float halfH = outH / 2.f;
        switch(anch->vAlign) {
          case AnchorV::Top: {
            outY = anch->padding.y + halfH;
            break;
          }
          case AnchorV::Bottom: {
            outY = scrH - anch->padding.y - halfH;
            break;
          }
          case AnchorV::Center: {
            outY = scrH / 2.f;
            break;
          }
          [[unreachable]] default: break;
        }
        
        kin->pos = {outX, outY};
        kin->scale = {outW, outH};
        
        if(auto* bar = bars.get(ae)) {
          bar->maxW = outW;
        }
      }
      
      // step 2: logic
      Health* ph = nullptr;
      Exp* pexp = nullptr;
      
      for(auto e : manager.view<PlayerTag>().getOwners()) {
        ph = manager.getComponent<Health>(e);
        pexp = manager.getComponent<Exp>(e);
        
        break;
      }
      
      if(!ph || !pexp) return;
      
      for(auto be : bars.getOwners()) {
        auto* kin = ks.get(be);
        auto* bar = bars.get(be);
        if(!kin || !bar) continue;
        
        float percent;
        switch(bar->bType) {
          case BarType::HP: {
            percent = std::max(ph->cur / ph->max, 0.f);
            break;
          }
          case BarType::EXP: {
            percent = std::max(pexp->cur / pexp->max, 0.f);
            break;
          }
          default: break;
        }
          kin->scale.x = bar->maxW * percent;
      }
    }
  };
  
  class AnimSystem : public ecs::ISystem {
  public:
    void update(ecs::Manager& manager, const float dT) override {
      auto& animators = manager.view<Animator>();
      auto& sprites = manager.view<Sprite>();
      auto& acts = manager.view<Active>();
      
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
        auto* active = manager.getComponent<Active>(i);
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
        auto* active = manager.getComponent<Active>(e);
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
      for(auto& e : manager.view<PlayerTag>().getOwners()) {
        playerPos = manager.getComponent<Kinematics>(e)->pos;
        break;
      }
      
      auto& tiles = manager.view<BgTile>();
      auto& ks = manager.view<Kinematics>();
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
        manager.getComponent<Active>(e)->value = true;
        auto* kin = manager.getComponent<Kinematics>(e);
        kin->pos = spawnPos;
        kin->speed = m_speed;
        manager.getComponent<Health>(e)->cur = manager.getComponent<Health>(e)->max;
        
        if(auto* se = manager.getComponent<StatusEffects>(e))
          manager.removeComponent<StatusEffects>(e);
        if(auto* fe = manager.getComponent<FlashEffect>(e))
          manager.removeComponent<FlashEffect>(e);
        if(auto* clr = manager.getComponent<ColorTint>(e)) {
          clr->curColor = clr->baseColor;
        }
        // manager.getComponent<Health>(e)->iFrames = 0.5f;
      }
      else {
        e = manager.createEntity();
      
        manager.addComponent(e, EnemyTag{});
        manager.addComponent(e, Active{});
        manager.addComponent(e, Kinematics{
          .pos = {spawnPos.x, spawnPos.y},
          .scale = {50.f, 50.f},
          .rot = 0.f,
          .speed = m_speed,
          .z = 9
        });
        manager.addComponent(e, CircleCollider{.radius = manager.getComponent<Kinematics>(e)->scale.x / 2.f});
        manager.addComponent(e, Health{
          .max = 30.f,
          .iFrames = 0.5f
        });
        manager.getComponent<Health>(e)->cur = manager.getComponent<Health>(e)->max;
        manager.addComponent(e, Sprite{
          .mesh = m_rend->getGlobalQuad(),
          .material = m_enemyMat
        });
        manager.addComponent(e, ColorTint{});
        
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
      for(auto e : manager.view<PlayerTag>().getOwners()) {
        plPos = manager.getComponent<Kinematics>(e)->pos;
        break;
      }
      
      for(int i = 0; i < count; ++i) {
        float angle = (360.f / count) * i;
        float rad = glm::radians(angle);
        
        glm::vec2 spawnPos = plPos + glm::vec2(cos(rad), sin(rad)) * m_spawnRadius;
        
        ecs::EntID e = createEnemy(manager, spawnPos);
        
        // auto* vel = manager.getComponent<Velocity>(e);
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
      for(auto e : manager.view<PlayerTag>().getOwners()) {
        plPos = manager.getComponent<Kinematics>(e)->pos;
        break;
      }
      auto& es = manager.view<EnemyTag>();
      for(auto e : es.getOwners()) {
        auto* act = manager.getComponent<Active>(e);
        if(act && !act->value) continue;
        auto* kin = manager.getComponent<Kinematics>(e);
        
        glm::vec2 dir = plPos - kin->pos;
        if(glm::length(dir) > 0.0001f) kin->vel = glm::normalize(dir) * kin->speed;
        else kin->vel = {0.f, 0.f};
      }
      
      // check HP
      auto& healths = manager.view<Health>();
      for (auto e : manager.view<EnemyTag>().getOwners()) {
        auto* active = manager.getComponent<Active>(e);
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
    
    std::vector<ecs::EntID> m_toDestroy;
    
  public:
    void update(ecs::Manager& manager, const float dT) override {
      m_toDestroy.clear();
      
      auto& healths = manager.view<Health>();
      auto& enemies = manager.view<EnemyTag>();
      auto& circles = manager.view<CircleCollider>();
      auto& ks = manager.view<Kinematics>();
      auto& dds = manager.view<DamageDealer>();
      auto& acts = manager.view<Active>();
      auto& pulses = manager.view<PulseCooldown>();
      auto& players = manager.view<PlayerTag>();
      
      
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
            manager.addComponent(ee, FlashEffect{ .maxTime = time, .curTime = time, .color = {1.f, 0.f, 0.f, 1.f} });
            
            if(auto* applies = manager.getComponent<AppliesDoT>(we)) {
              auto* statuses = manager.getComponent<StatusEffects>(ee);
              if(!statuses) {
                statuses = &manager.addComponent(ee, StatusEffects{});
              }
              statuses->dots.emplace_back(DoTCharge{
                .damage = applies->dmgPerTick,
                .tickRate = applies->tickRate,
                .curTickTimer = applies->tickRate,
                .lifetime = applies->duration
              });
            }
            
            if(auto* pierce = manager.getComponent<Pierce>(we)) {
              pierce->count--;
              if(pierce->count <= 0) {
                // wact->value = false;
                m_toDestroy.emplace_back(we);
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
            if(auto* spr = manager.getComponent<Sprite>(pe)) {
              // flash effect after gaining damage
              float time = 0.3f;
              manager.addComponent(pe, FlashEffect{ .maxTime = time, .curTime = time, .color = {1.f, 0.f, 0.f, 1.f} });
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
      for (auto we : m_toDestroy) {
        manager.destroyEntity(we);
      }
      
    }
    
  };
  
  class StatusSystem : public ecs::ISystem {
  public:
    void update(ecs::Manager& manager, const float dT) override {
      auto& healths = manager.view<Health>();
      auto& statuses = manager.view<StatusEffects>();
      auto& acts = manager.view<Active>();
      
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
      auto& acts = manager.view<Active>();
      auto& atts = manager.view<AttachTo>().getOwners();
      for(auto e : atts) {
        auto* act = acts.get(e);
        if(act && !act->value) continue;
        auto te = manager.getComponent<AttachTo>(e)->target;
        auto* ks = manager.getComponent<Kinematics>(te);
        auto* itsKs = manager.getComponent<Kinematics>(e);
        if(ks && itsKs)
          itsKs->pos = ks->pos + manager.getComponent<AttachTo>(e)->offset;
      }
    }
  };
  
  class LifetimeSystem : public ecs::ISystem {
    
    std::vector<ecs::EntID> m_toDestroy;
    
    void update(ecs::Manager& manager, const float dT) override {
      m_toDestroy.clear();
      
      auto& acts = manager.view<Active>();
      for(auto e : manager.view<Lifetime>().getOwners()) {
        auto* act = acts.get(e);
        if(act && !act->value) continue;
        auto* te = manager.getComponent<Lifetime>(e);
        if(te) {
          te->curTimer -= dT;
          if(te->curTimer <= 0.f) m_toDestroy.emplace_back(e);
        }
        if(te->curTimer <= 0) act->value = false;
      }
      
      for(auto e : m_toDestroy) {
        manager.destroyEntity(e);
      }
    }
  };
  
}; //game