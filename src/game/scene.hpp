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
    float scrW, scrH;
    
    bool regComponents() {
      m_manager->registerComponent<game::BgTile>();
      m_manager->registerComponent<game::Script>();
      
      m_manager->registerComponent<game::PlayerTag>();
      m_manager->registerComponent<game::Kinematics>();
      m_manager->registerComponent<game::Sprite>();
      m_manager->registerComponent<game::ColorTint>();
      m_manager->registerComponent<game::FlashEffect>();
      m_manager->registerComponent<game::Animator>();
      
      m_manager->registerComponent<game::Exp>();
      m_manager->registerComponent<game::GameState>();
      m_manager->registerComponent<game::UITag>();
      m_manager->registerComponent<game::UIProgressBar>();
      m_manager->registerComponent<game::BarType>();
      m_manager->registerComponent<game::UIAnchor>();
      m_manager->registerComponent<game::AnchorH>();
      m_manager->registerComponent<game::AnchorV>();
      
      m_manager->registerComponent<game::EnemyTag>();
      m_manager->registerComponent<game::WeaponTag>();
      m_manager->registerComponent<game::Active>();
      m_manager->registerComponent<game::CircleCollider>();
      m_manager->registerComponent<game::Health>();
      m_manager->registerComponent<game::DamageDealer>();
      m_manager->registerComponent<game::PulseCooldown>();
      m_manager->registerComponent<game::Lifetime>();
      m_manager->registerComponent<game::AttachTo>();
      m_manager->registerComponent<game::Pierce>();
      
      return true;
    }
    bool regAssets(mip::IRenderer* rend) {
      game::TextureLoader tLoader{rend};
      m_manager->registerAsset<std::shared_ptr<mip::ITexture>>(tLoader);
      
      return true;
    }
    bool regSystems(GLFWwindow* wnd, mip::IRenderer* rend) {
      m_manager->registerSystem<game::PlayerControllerSystem>(wnd);
      m_manager->registerSystem<game::TileSystem>();
      // m_manager->registerSystem<game::PatrolSystem>();
      m_manager->registerSystem<game::MovementSystem>();
      m_manager->registerSystem<game::AttachmentSystem>();
      m_manager->registerSystem<game::DamageSystem>();
      m_manager->registerSystem<game::LifetimeSystem>();
      m_manager->registerSystem<game::VisualEffectsSystem>();
      m_manager->registerSystem<game::AnimSystem>();
      m_manager->registerSystem<game::EnemySpawnerSystem>(rend);
      m_manager->registerSystem<game::UISystem>(wnd);
      m_manager->registerSystem<game::GamePlayUISystem>();
      m_manager->registerSystem<game::RenderSystem>(rend);
      
      return true;
    }
    
    bool createBars(float x, float y, ecs::EntID player, mip::IRenderer* rend) {
      auto uiMat = rend->createMaterial("../../assets/shaders/shader.spv");
      auto whiteTexHandle = m_manager->loadAsset<std::shared_ptr<mip::ITexture>>("../../assets/textures/whitepixel.png");
      if (auto tex = m_manager->getAsset(whiteTexHandle)) uiMat->setTexture(0, *tex);
      else {
        Logger::error("Failed to create ui material!");
        return false;
      }
      
      // hp
      auto hpBg = m_manager->createEntity();
      auto hpAtt = m_manager->addComponent(hpBg, game::AttachTo{ .target = player, .offset = {-30.f, -60.f} });
      auto hpKin = m_manager->addComponent(hpBg, game::Kinematics{ .scale = {60.f, 8.f}, .z = 99 });
      m_manager->addComponent(hpBg, game::ColorTint{ .baseColor = {0.1f, 0.1f, 0.1f, 1.f} });
      m_manager->addComponent(hpBg, game::Sprite{ .mesh = rend->getUIQuad(), .material = uiMat });

      auto hpFill = m_manager->createEntity();
      m_manager->addComponent(hpFill, game::UIProgressBar{ .bType = game::BarType::HP, .maxW = 60.f });
      m_manager->addComponent(hpFill, game::AttachTo{hpAtt});
      m_manager->addComponent(hpFill, game::Kinematics{ .scale = hpKin.scale, .z = 100 });
      m_manager->addComponent(hpFill, game::ColorTint{ .baseColor = {1.f, 0.f, 0.f, 1.f} });
      m_manager->addComponent(hpFill, game::Sprite{ .mesh = rend->getUIQuad(), .material = uiMat });

      // exp
      game::UIAnchor expAnchor {
        .hAlign = game::AnchorH::Stretch, 
        .vAlign = game::AnchorV::Bottom, 
        .padding = {10.f, 10.f}
      };

      auto expBg = m_manager->createEntity();
      m_manager->addComponent(expBg, game::UITag{});
      m_manager->addComponent(expBg, game::UIAnchor{expAnchor});
      m_manager->addComponent(expBg, game::Kinematics{ .z = 99 });
      m_manager->addComponent(expBg, game::ColorTint{ .baseColor = {0.1f, 0.2f, 0.4f, 1.f} });
      m_manager->addComponent(expBg, game::Sprite{ .mesh = rend->getUIQuad(), .material = uiMat });

      auto expFill = m_manager->createEntity();
      m_manager->addComponent(expFill, game::UITag{});
      m_manager->addComponent(expFill, game::UIAnchor{expAnchor});
      m_manager->addComponent(expFill, game::UIProgressBar{ .bType = game::BarType::EXP });
      m_manager->addComponent(expFill, game::Kinematics{ .z = 100 });
      m_manager->addComponent(expFill, game::ColorTint{ .baseColor = {0.2f, 0.3f, 1.f, 1.f} });
      m_manager->addComponent(expFill, game::Sprite{ .mesh = rend->getUIQuad(), .material = uiMat });
      
      return true;
    }
    
  public:
    
    Scene() {
      m_manager = std::make_unique<ecs::Manager>();
    }
    
    bool init(mip::Window* wnd, mip::IRenderer* rend) {
      if(
           !regComponents()
        || !regAssets(rend)
        || !regSystems(wnd->getWindow(), rend)
      ) return false;
      
      Logger::info("Scene initialized successfully.");
      return true;
    }
    
    bool update(const float dT, mip::IRenderer* rend, const float w, const float h) {
      
      // ImGui
      auto* vkRend = static_cast<mip::VulkanRenderer*>(rend);
      vkRend->beginImGuiFrame();
      // ImGui::ShowDemoWindow();
      
      glm::vec2 playerPos{0.f, 0.f};
      auto& ks = m_manager->view<game::Kinematics>();
      auto& ps = m_manager->view<game::PlayerTag>();
      for(auto e : ps.getOwners()) {
        playerPos = ks.get(e)->pos;
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
      vkRend->renderImGui();
      if(!rend->endFrame()) return false;
      
      if(auto* ph = m_manager->getComponent<game::Health>(ps.getOwners()[0]); ph->cur <= 0) {
        Logger::debug("Player died!");
        return false;
      }
      
      return true;
    }
    
    bool createLevel(float width, float height, mip::IRenderer* rend, const std::string& txtrPath) {
      auto map = m_manager->createEntity();
      m_manager->addComponent(map, game::Kinematics{
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
    bool createTileLevel(mip::IRenderer* rend, const std::string& txtrPath) {
      auto tileTxtr = m_manager->loadAsset<std::shared_ptr<mip::ITexture>>(txtrPath);
      auto tileMat = rend->createMaterial("../../assets/shaders/shader.spv");
      if(!tileMat) {
        Logger::error("Failed to create material for sprite!");
        return false;
      }
      if (auto tex = m_manager->getAsset(tileTxtr)) {
        tileMat->setTexture(0, *tex);
      }
      
      for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
          auto tile = m_manager->createEntity();
          m_manager->addComponent(tile, game::Kinematics{
            .pos = {0.f, 0.f},
            .scale = {game::TileSystem::tileSize, game::TileSystem::tileSize},
            .z = 0
          });
          m_manager->addComponent(tile, game::BgTile{.offset = {static_cast<float>(x), static_cast<float>(y)}});
          m_manager->addComponent(tile, game::Sprite{
            .mesh = rend->getGlobalQuad(),
            .material = tileMat
          });
        }
      }
      
      return true;
    }
    bool createPlayer(float x, float y, mip::IRenderer* rend, const std::string& txtrPath) {
      auto player = m_manager->createEntity();
      m_manager->addComponent(player, game::PlayerTag{});
      m_manager->addComponent(player, game::GameState{});
      m_manager->addComponent(player, game::Kinematics{
        .pos = {x, y},
        .scale = {100.f, 100.f},
        .rot = 0.f,
        .speed = 300.f,
        .z = 10
      });
      auto pCol = m_manager->addComponent(player, game::CircleCollider{.radius = m_manager->getComponent<Kinematics>(player)->scale.x / 2.f});
      m_manager->addComponent(player, game::Health{.max = 100.f});
      m_manager->getComponent<game::Health>(player)->cur = m_manager->getComponent<game::Health>(player)->max;
      
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
        .material = playerMat,
      });
      m_manager->addComponent(player, game::ColorTint{});
      m_manager->addComponent(player, game::Exp{.cur = 5.f});
      
      if(!createBars(x, y, player, rend)) return false;
      
      m_manager->addComponent(player, game::Animator{
        .cols = 4,
        .rows = 1,
        .startFrame = 0,
        .frameCnt = 4,
        .frameTime = 0.15f
      });
      
      
      auto aura = m_manager->createEntity();
      auto ps = m_manager->view<game::PlayerTag>().getOwners()[0];
      m_manager->addComponent(aura, game::WeaponTag{});
      m_manager->addComponent(aura, game::DamageDealer{.amount = 10.f});
      m_manager->addComponent(aura, game::PulseCooldown{.maxTimer = 0.5});
      m_manager->getComponent<game::PulseCooldown>(aura)->curTimer = m_manager->getComponent<game::PulseCooldown>(aura)->maxTimer;
      m_manager->addComponent(aura, game::Kinematics{
        .pos = m_manager->getComponent<game::Kinematics>(ps)->pos,
        .scale = {500.f, 500.f},
        .z = 8
      });
      m_manager->addComponent(aura, game::AttachTo{.target = ps});
      m_manager->addComponent(aura, game::CircleCollider{.radius = m_manager->getComponent<Kinematics>(aura)->scale.x / 2.f});
      auto auraTex = m_manager->loadAsset<std::shared_ptr<mip::ITexture>>("../../assets/textures/222.png");
      auto auraMat = rend->createMaterial("../../assets/shaders/shader.spv");
      if(!auraMat) {
        Logger::error("Failed to create material for aura!");
        return false;
      }
      if (auto tex = m_manager->getAsset(auraTex)) {
        auraMat->setTexture(0, *tex);
      }
      m_manager->addComponent(aura, game::Sprite{
        .mesh = rend->getGlobalQuad(),
        .material = auraMat
      });
      
      
      return true;
    }
    bool createMobs(float x, float y, mip::IRenderer* rend, const std::string& txtrPath) {
      auto mob = m_manager->createEntity();
      m_manager->addComponent(mob, game::Kinematics{
        .pos = {x, y},
        .scale = {50, 50},
        .rot = 0.f,
        .z = 1
      });
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