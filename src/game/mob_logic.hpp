#pragma once

#include "../common/ecs_core.hpp"
#include "components.hpp"

namespace game {
  
  inline Task squarePatrol(ecs::Manager& manager, ecs::EntID e, float moveTime, float waitTime, float speed) {
    while(true) {
      auto* v = manager.getComponent<Velocity>(e);
      if(!v) co_return;
      v->vel = {speed, 0.f};
      co_yield moveTime;
      
      v->vel = {0.f, 0.f};
      co_yield waitTime;
      
      v->vel = {0.f, speed};
      co_yield moveTime;
      
      v->vel = {0.f, 0.f};
      co_yield waitTime;
      
      v->vel = {-speed, 0.f};
      co_yield moveTime;
      
      v->vel = {0.f, 0.f};
      co_yield waitTime;
      
      v->vel = {0.f, -speed};
      co_yield moveTime;
      
      v->vel = {0.f, 0.f};
      co_yield waitTime;
    }
  }
  
}; //game