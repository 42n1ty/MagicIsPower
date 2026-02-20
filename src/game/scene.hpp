#pragma once

#include "../graphics/renderer/vulkan/vk_renderer.hpp"
#include "../game/components.hpp"
#include "../game/loaders.hpp"
#include "../game/systems.hpp"
#include "../game/mob_logic.hpp"

class GLFWwindow;


namespace game {
  
  class Scene {
    
    std::unique_ptr<ecs::Manager> m_manager = nullptr;
    
    bool regComponents() {
      m_manager->registerComponent<game::Transform>();
      m_manager->registerComponent<game::Sprite>();
      m_manager->registerComponent<game::Velocity>();
      m_manager->registerComponent<game::Script>();
      m_manager->registerComponent<game::PlayerTag>();
      
      return true;
    }
    bool regAssets(mip::IRenderer* rend) {
      game::TextureLoader tLoader{rend};
      m_manager->registerAsset<std::shared_ptr<mip::ITexture>>(tLoader);
      
      return true;
    }
    bool regSystems(GLFWwindow* wnd, mip::IRenderer* rend) {
      m_manager->registerSystem<game::PlayerControllerSystem>(wnd);
      m_manager->registerSystem<game::PatrolSystem>();
      m_manager->registerSystem<game::MovementSystem>();
      m_manager->registerSystem<game::RenderSystem>(rend);
      
      return true;
    }
    
  public:
    
    Scene() {
      m_manager = std::make_unique<ecs::Manager>();
    }
    
    bool init(GLFWwindow* wnd, mip::IRenderer* rend) {
      if(
           !regComponents()
        || !regAssets(rend)
        || !regSystems(wnd, rend)
      ) return false;
      
      Logger::info("Scene initialized successfully.");
      return true;
    }
    
    bool update(const float dT, mip::IRenderer* rend, const float w, const float h) {
      
      glm::vec2 playerPos{0.f, 0.f};
      auto& ts = m_manager->view<game::Transform>();
      auto& ps = m_manager->view<game::PlayerTag>();
      for(auto e : ps.getOwners()) {
        playerPos = ts.get(e)->pos;
        break;
      }
      mip::CameraInfo camData{};
      camData.projection = glm::ortho(0.f, static_cast<float>(w), static_cast<float>(h), 0.f, -1.f, 1.f);
      float centerX = w / 2.f;
      float centerY = h / 2.f;
      glm::mat4 view = glm::mat4(1.f);
      view = glm::translate(view, glm::vec3(centerX, centerY, 0.f));
      view = glm::translate(view, glm::vec3(-playerPos.x, -playerPos.y, 0.f));
      camData.view = view;
      
      if(!rend->beginFrame(camData)) return false;
      m_manager->update(dT);
      if(!rend->endFrame()) return false;
      
      return true;
    }
    
    bool createLevel(float width, float height, mip::IRenderer* rend, const std::string& txtrPath) {
      auto map = m_manager->createEntity();
      m_manager->addComponent(map, game::Transform{
        .pos = {width / 2, height / 2},
        .scale = {width, height},
        .rot = 0.f,
        .z = 0
      });
      auto texHandle = m_manager->loadAsset<std::shared_ptr<mip::ITexture>>(txtrPath);
      auto mapMat = rend->createMaterial("../../assets/shaders/shader.spv");
      if(!mapMat) {
        Logger::error("Failed to create material for sprite!");
        return false;
      }
      if (auto tex = m_manager->getAsset(texHandle)) {
        mapMat->setTexture(0, *tex);
      }
      m_manager->addComponent(map, game::Sprite{
        .mesh = rend->getGlobalQuad(),
        .material = mapMat
      });
      
      return true;
    }
    bool createPlayer(float x, float y, mip::IRenderer* rend, const std::string& txtrPath) {
      auto player = m_manager->createEntity();
      m_manager->addComponent(player, game::PlayerTag{});
      m_manager->addComponent(player, game::Transform{
        .pos = {x, y},
        .scale = {100, 100},
        .rot = 0.f,
        .z = 2
      });
      auto& v = m_manager->addComponent(player, game::Velocity{});
      
      auto texHandle = m_manager->loadAsset<std::shared_ptr<mip::ITexture>>(txtrPath);
      auto playerMat = rend->createMaterial("../../assets/shaders/shader.spv");
      if(!playerMat) {
        Logger::error("Failed to create material for player!");
        return false;
      }
      if (auto tex = m_manager->getAsset(texHandle)) {
        playerMat->setTexture(0, *tex);
      }
      m_manager->addComponent(player, game::Sprite{
        .mesh = rend->getGlobalQuad(),
        .material = playerMat
      });
      
      return true;
    }
    bool createMobs(float x, float y, mip::IRenderer* rend, const std::string& txtrPath) {
      auto mob = m_manager->createEntity();
      m_manager->addComponent(mob, game::Transform{
        .pos = {x, y},
        .scale = {50, 50},
        .rot = 0.f,
        .z = 1
      });
      auto& v = m_manager->addComponent(mob, game::Velocity{});
      m_manager->addComponent(mob, game::Script{.task = game::squarePatrol(*m_manager, mob, 2.f, 3.f, 150.f)});
      
      auto texHandle = m_manager->loadAsset<std::shared_ptr<mip::ITexture>>(txtrPath);
      auto mobMat = rend->createMaterial("../../assets/shaders/shader.spv");
      if(!mobMat) {
        Logger::error("Failed to create material for mob!");
        return false;
      }
      if (auto tex = m_manager->getAsset(texHandle)) {
        mobMat->setTexture(0, *tex);
      }
      m_manager->addComponent(mob, game::Sprite{
        .mesh = rend->getGlobalQuad(),
        .material = mobMat
      });
      
      return true;
    }
    
  };
  
}; //game