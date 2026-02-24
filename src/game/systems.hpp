#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "../common/ecs_core.hpp"
#include "components.hpp"
#include "../graphics/i_renderer.hpp"
#include "../graphics/i_material.hpp"


namespace game {
  
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
      auto& ts = manager.view<Transform>();
      
      for(ecs::EntID entity : sprites.getOwners()) {
        Transform* t = ts.get(entity);
        if(!t) continue;
        
        rendQ.push_back({entity, t->z});
      }
      
      std::sort(rendQ.begin(), rendQ.end(), [](const auto& a, const auto& b) {
        return a.z < b.z;
      });
      
      for(const auto& item : rendQ) {
        auto* active = manager.getComponent<game::Active>(item.e);
        if(active && !active->value) continue;
        auto* spr = sprites.get(item.e);
        auto* t = ts.get(item.e);
        
        glm::mat4 model = glm::mat4(1.f);
        model = glm::translate(model, glm::vec3(t->pos, 0.f));
        model = glm::rotate(model, glm::radians(t->rot), glm::vec3(0.f, 0.f, 1.f));
        model = glm::scale(model, glm::vec3(t->scale, 1.f));
        
        renderer->submit(spr->mesh, spr->material, model);
      }
      
    }
  };
  
  class MovementSystem : public ecs::ISystem {
  public:
    MovementSystem() {}
    
    void update(ecs::Manager& manager, const float dT) override {
      auto& sonics = manager.view<Velocity>();
      auto& ts = manager.view<Transform>();
      
      for(ecs::EntID e : sonics.getOwners()) {
        auto* active = manager.getComponent<game::Active>(e);
        if(active && !active->value) continue;
        
        auto* t = ts.get(e);
        if(!t) continue;
        
        auto* vel = sonics.get(e);
        t->pos += vel->value * dT;
      }
      
    }
  };
  
  class PlayerControllerSystem : public ecs::ISystem {
    GLFWwindow* m_wnd;
    float speed = 300.f;
    
  public:
    PlayerControllerSystem(GLFWwindow* wnd) : m_wnd(wnd) {}
    
    void update(ecs::Manager& manager, const float dT) override{
      auto& vs = manager.view<Velocity>();
      auto& ps = manager.view<PlayerTag>();
      
      for(ecs::EntID e : ps.getOwners()) {
        auto* vel = vs.get(e);
        if(!vel) continue;
        
        glm::vec2 moveDir{0.f, 0.f};
        
        if (glfwGetKey(m_wnd, GLFW_KEY_W) == GLFW_PRESS) moveDir.y -= 1.f;
        if (glfwGetKey(m_wnd, GLFW_KEY_S) == GLFW_PRESS) moveDir.y += 1.f;
        if (glfwGetKey(m_wnd, GLFW_KEY_A) == GLFW_PRESS) moveDir.x -= 1.f;
        if (glfwGetKey(m_wnd, GLFW_KEY_D) == GLFW_PRESS) moveDir.x += 1.f;
        
        if(glm::length(moveDir) > 0.f) {
          moveDir = glm::normalize(moveDir);
        }
        
        vel->value = moveDir * speed;
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
        playerPos = manager.getComponent<game::Transform>(e)->pos;
        break;
      }
      
      auto& tiles = manager.view<game::BgTile>();
      auto& ts = manager.view<game::Transform>();
      for(auto& e : tiles.getOwners()) {
        auto* tile = tiles.get(e);
        auto* t = ts.get(e);
        float targetX = std::round(playerPos.x / tileSize) * tileSize + tile->offset.x * tileSize;
        float targetY = std::round(playerPos.y / tileSize) * tileSize + tile->offset.y * tileSize;
        
        t->pos = {targetX, targetY};
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
        manager.getComponent<game::Transform>(e)->pos = spawnPos;
        manager.getComponent<game::Health>(e)->cur = manager.getComponent<game::Health>(e)->max;
        manager.getComponent<game::Health>(e)->iFrames = 0.2f;
      }
      else {
        e = manager.createEntity();
      
        manager.addComponent(e, game::EnemyTag{});
        manager.addComponent(e, game::Active{});
        manager.addComponent(e, game::Transform{
          .pos = {spawnPos.x, spawnPos.y},
          .scale = {50, 50},
          .rot = 0.f,
          .z = 9
        });
        manager.addComponent(e, game::Velocity{});
        manager.addComponent(e, game::CircleCollider{.radius = 27.f});
        manager.addComponent(e, game::Health{
          .max = 30,
          .iFrames = 0.2f
        });
        manager.getComponent<game::Health>(e)->cur = manager.getComponent<game::Health>(e)->max;
        manager.addComponent(e, game::Sprite{
          .mesh = m_rend->getGlobalQuad(),
          .material = m_enemyMat
        });
        
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
        plPos = manager.getComponent<game::Transform>(e)->pos;
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
        plPos = manager.getComponent<game::Transform>(e)->pos;
        break;
      }
      auto& es = manager.view<game::EnemyTag>();
      for(auto e : es.getOwners()) {
        auto* act = manager.getComponent<game::Active>(e);
        if(act && !act->value) continue;
        auto* vel = manager.getComponent<game::Velocity>(e);
        auto* ts = manager.getComponent<game::Transform>(e);
        vel->value = glm::normalize(plPos - ts->pos) * m_speed;
      }
      
      // check HP
      auto& healths = manager.view<game::Health>();
      for (auto e : manager.view<game::EnemyTag>().getOwners()) {
        auto* active = manager.getComponent<game::Active>(e);
        if (active->value && healths.get(e)->cur <= 0) {
          active->value = false;
          m_pool.push_back(e);
          // exp
        }
      }
    }
    
  };
  
  class DamageSystem : public ecs::ISystem {
  public:
    void update(ecs::Manager& manager, const float dT) override {
      auto& healths = manager.view<game::Health>();
      auto& circles = manager.view<game::CircleCollider>();
      auto& ts = manager.view<game::Transform>();
      auto& dds = manager.view<game::DamageDealer>();
      auto& acts = manager.view<game::Active>();
      
      for(auto e : healths.getOwners()) {
        auto* act = acts.get(e);
        if(act && !act->value) continue;
        auto* hp = healths.get(e);
        if(hp->iFrames > 0.f) hp->iFrames -= dT;
      }
      
      for(auto we : dds.getOwners()) {
        auto* wact = acts.get(we);
        if(wact && !wact->value) continue;
        auto* wc = circles.get(we);
        auto* wt = ts.get(we);
        auto* dmg = dds.get(we);
        
        if(!wc || !wt) continue;
        
        for(auto ee : healths.getOwners()) {
          auto* eact = acts.get(ee);
          if(eact && !eact->value) continue;
          auto* ehp = healths.get(ee);
          auto* ec = circles.get(ee);
          auto* et = ts.get(ee);
          
          if(!ec || !et) continue;
          if(ehp->iFrames > 0.f) continue;
          
          float dist = glm::distance(wt->pos, et->pos);
          if(dist < (wc->radius + ec->radius)) {
            ehp->cur -= dmg->amount;
            ehp->iFrames = 0.2f;
            
            if(auto* pierce = manager.getComponent<game::Pierce>(we)) {
              pierce->count--;
              if(pierce->count <= 0) {
                // wact->value = false;
                manager.destroyEntity(we);
                break;
              }
            }
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
        auto* ts = manager.getComponent<game::Transform>(te);
        auto* itsTs = manager.getComponent<game::Transform>(e);
        if(ts && itsTs) itsTs->pos = ts->pos;
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
        if(te && te->timer > 0) te->timer -= dT;
        if(te->timer <= 0) act->value = false;
      }
    }
  };
  
}; //game