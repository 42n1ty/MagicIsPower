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
        auto* t = ts.get(e);
        if(!t) continue;
        
        auto* vel = sonics.get(e);
        t->pos += vel->vel * dT;
      }
      
    }
  };
  
  class PlayerControllerSystem : public ecs::ISystem {
    GLFWwindow* wnd;
    float speed = 300.f;
    
  public:
    PlayerControllerSystem(GLFWwindow* wnd) : wnd(wnd) {}
    
    void update(ecs::Manager& manager, const float dT) override{
      auto& vs = manager.view<Velocity>();
      auto& ps = manager.view<PlayerTag>();
      
      for(ecs::EntID e : ps.getOwners()) {
        auto* vel = vs.get(e);
        if(!vel) continue;
        
        glm::vec2 moveDir{0.f, 0.f};
        
        if (glfwGetKey(wnd, GLFW_KEY_W) == GLFW_PRESS) moveDir.y -= 1.f;
        if (glfwGetKey(wnd, GLFW_KEY_S) == GLFW_PRESS) moveDir.y += 1.f;
        if (glfwGetKey(wnd, GLFW_KEY_A) == GLFW_PRESS) moveDir.x -= 1.f;
        if (glfwGetKey(wnd, GLFW_KEY_D) == GLFW_PRESS) moveDir.x += 1.f;
        
        if(glm::length(moveDir) > 0.f) {
          moveDir = glm::normalize(moveDir);
        }
        
        vel->vel = moveDir * speed;
      }
    }
  };
  
}; //game