#pragma once

#include "../common/ecs_core.hpp"
#include "components.hpp"

namespace game {
  
  inline Task squarePatrol(ecs::Manager& manager, ecs::EntID e, float moveTime, float waitTime, float speed) {
    while(true) {
      auto* k = manager.getComponent<Kinematics>(e);
      if(!k) co_return;
      k->vel = {speed, 0.f};
      co_yield moveTime;
      
      k->vel = {0.f, 0.f};
      co_yield waitTime;
      
      k->vel = {0.f, speed};
      co_yield moveTime;
      
      k->vel = {0.f, 0.f};
      co_yield waitTime;
      
      k->vel = {-speed, 0.f};
      co_yield moveTime;
      
      k->vel = {0.f, 0.f};
      co_yield waitTime;
      
      k->vel = {0.f, -speed};
      co_yield moveTime;
      
      k->vel = {0.f, 0.f};
      co_yield waitTime;
    }
  }
  
}; //game