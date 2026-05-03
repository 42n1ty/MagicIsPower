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
    std::unique_ptr<SkillDB> m_skillDB = nullptr;
    float scrW, scrH;
    
    bool regComponents() {
      m_manager->registerComponent<BgTile>();
      m_manager->registerComponent<Script>();
      
      m_manager->registerComponent<PlayerTag>();
      m_manager->registerComponent<DirtyStatsTag>();
      m_manager->registerComponent<SkillTag>();
      m_manager->registerComponent<InventoryItem>();
      m_manager->registerComponent<PlayerStats>();
      m_manager->registerComponent<PermanentStats>();
      m_manager->registerComponent<ActiveSkillGem>();
      m_manager->registerComponent<SupGem>();
      m_manager->registerComponent<LinkedGems>();
      m_manager->registerComponent<Kinematics>();
      m_manager->registerComponent<Sprite>();
      m_manager->registerComponent<ColorTint>();
      m_manager->registerComponent<FlashEffect>();
      m_manager->registerComponent<Animator>();
      
      m_manager->registerComponent<Exp>();
      m_manager->registerComponent<GameState>();
      m_manager->registerComponent<UITag>();
      m_manager->registerComponent<UIProgressBar>();
      m_manager->registerComponent<BarType>();
      m_manager->registerComponent<UIAnchor>();
      m_manager->registerComponent<AnchorH>();
      m_manager->registerComponent<AnchorV>();
      
      m_manager->registerComponent<EnemyTag>();
      m_manager->registerComponent<WeaponTag>();
      m_manager->registerComponent<Active>();
      m_manager->registerComponent<CircleCollider>();
      m_manager->registerComponent<Health>();
      m_manager->registerComponent<DamageDealer>();
      m_manager->registerComponent<Resistances>();
      m_manager->registerComponent<PulseCooldown>();
      m_manager->registerComponent<Lifetime>();
      m_manager->registerComponent<AttachTo>();
      m_manager->registerComponent<Pierce>();
      
      return true;
    }
    bool regAssets(mip::IRenderer* rend) {
      TextureLoader tLoader{rend};
      m_manager->registerAsset<std::shared_ptr<mip::ITexture>>(tLoader);
      
      return true;
    }
    bool regSystems(GLFWwindow* wnd, mip::IRenderer* rend) {
      m_manager->registerSystem<PlayerControllerSystem>(wnd);
      m_manager->registerSystem<TileSystem>();
      // m_manager->registerSystem<PatrolSystem>();
      m_manager->registerSystem<MovementSystem>();
      m_manager->registerSystem<AttachmentSystem>();
      m_manager->registerSystem<LifetimeSystem>();
      m_manager->registerSystem<StatCalcSystem>(m_skillDB.get());
      m_manager->registerSystem<DamageSystem>();
      m_manager->registerSystem<CombatSystem>(m_skillDB.get(), wnd);
      m_manager->registerSystem<VisualEffectsSystem>();
      m_manager->registerSystem<AnimSystem>();
      m_manager->registerSystem<EnemySpawnerSystem>(rend);
      m_manager->registerSystem<UISystem>(wnd);
      m_manager->registerSystem<GamePlayUISystem>();
      m_manager->registerSystem<RenderSystem>(rend);
      
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
      auto hpAtt = m_manager->addComponent(hpBg, AttachTo{ .target = player, .offset = {-30.f, -60.f} });
      auto hpKin = m_manager->addComponent(hpBg, Kinematics{ .z = 99, .scale = {60.f, 8.f} });
      m_manager->addComponent(hpBg, ColorTint{ .baseColor = {0.1f, 0.1f, 0.1f, 1.f} });
      m_manager->addComponent(hpBg, Sprite{ .mesh = rend->getUIQuad(), .material = uiMat });

      auto hpFill = m_manager->createEntity();
      m_manager->addComponent(hpFill, UIProgressBar{ .bType = BarType::HP, .maxW = 60.f });
      m_manager->addComponent(hpFill, AttachTo{hpAtt});
      m_manager->addComponent(hpFill, Kinematics{ .z = 100, .scale = hpKin.scale });
      m_manager->addComponent(hpFill, ColorTint{ .baseColor = {1.f, 0.f, 0.f, 1.f} });
      m_manager->addComponent(hpFill, Sprite{ .mesh = rend->getUIQuad(), .material = uiMat });

      // exp
      UIAnchor expAnchor {
        .hAlign = AnchorH::Stretch, 
        .vAlign = AnchorV::Bottom, 
        .padding = {10.f, 10.f}
      };

      auto expBg = m_manager->createEntity();
      m_manager->addComponent(expBg, UITag{});
      m_manager->addComponent(expBg, UIAnchor{expAnchor});
      m_manager->addComponent(expBg, Kinematics{ .z = 99 });
      m_manager->addComponent(expBg, ColorTint{ .baseColor = {0.1f, 0.2f, 0.4f, 1.f} });
      m_manager->addComponent(expBg, Sprite{ .mesh = rend->getUIQuad(), .material = uiMat });

      auto expFill = m_manager->createEntity();
      m_manager->addComponent(expFill, UITag{});
      m_manager->addComponent(expFill, UIAnchor{expAnchor});
      m_manager->addComponent(expFill, UIProgressBar{ .bType = BarType::EXP });
      m_manager->addComponent(expFill, Kinematics{ .z = 100 });
      m_manager->addComponent(expFill, ColorTint{ .baseColor = {0.2f, 0.3f, 1.f, 1.f} });
      m_manager->addComponent(expFill, Sprite{ .mesh = rend->getUIQuad(), .material = uiMat });
      
      return true;
    }
    
  public:
    
    Scene() {
      m_manager = std::make_unique<ecs::Manager>();
    }
    
    bool init(mip::Window* wnd, mip::IRenderer* rend) {
      m_skillDB = std::make_unique<SkillDB>(rend);
      
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
      auto& ks = m_manager->view<Kinematics>();
      auto& ps = m_manager->view<PlayerTag>();
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
      
      if(auto* ph = m_manager->getComponent<Health>(ps.getOwners()[0]); ph->cur <= 0) {
        Logger::debug("Player died!");
        return false;
      }
      
      return true;
    }
    
    bool createLevel(float width, float height, mip::IRenderer* rend, const std::string& txtrPath) {
      auto map = m_manager->createEntity();
      m_manager->addComponent(map, Kinematics{
        .z = 0,
        .pos = {width / 2, height / 2},
        .scale = {width, height},
        .rot = 0.f
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
      m_manager->addComponent(map, Sprite{
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
          m_manager->addComponent(tile, Kinematics{
            .z = 0,
            .pos = {0.f, 0.f},
            .scale = {TileSystem::tileSize, TileSystem::tileSize}
          });
          m_manager->addComponent(tile, BgTile{.offset = {static_cast<float>(x), static_cast<float>(y)}});
          m_manager->addComponent(tile, Sprite{
            .mesh = rend->getGlobalQuad(),
            .material = tileMat
          });
        }
      }
      
      return true;
    }
    bool createPlayer(float x, float y, mip::IRenderer* rend, const std::string& txtrPath) {
      auto player = m_manager->createEntity();
      m_manager->addComponent(player, PlayerTag{});
      m_manager->addComponent(player, GameState{});
      m_manager->addComponent(player, PlayerStats{});
      m_manager->addComponent(player, PermanentStats{});
      m_manager->addComponent(player, Kinematics{
        .z = 10,
        .pos = {x, y},
        .scale = {100.f, 100.f},
        .rot = 0.f,
        .speed = 300.f
      });
      auto pCol = m_manager->addComponent(player, CircleCollider{.radius = m_manager->getComponent<Kinematics>(player)->scale.x / 2.f});
      m_manager->addComponent(player, Health{.max = 100.f});
      m_manager->getComponent<Health>(player)->cur = m_manager->getComponent<Health>(player)->max;
      
      auto texHandle = m_manager->loadAsset<std::shared_ptr<mip::ITexture>>(txtrPath);
      auto playerMat = rend->createMaterial("../../assets/shaders/shader.spv");
      if(!playerMat) {
        Logger::error("Failed to create material for player!");
        return false;
      }
      if (auto tex = m_manager->getAsset(texHandle)) {
        playerMat->setTexture(0, *tex);
      }
      m_manager->addComponent(player, Sprite{
        .mesh = rend->getGlobalQuad(),
        .material = playerMat,
      });
      m_manager->addComponent(player, ColorTint{});
      m_manager->addComponent(player, Exp{.cur = 5.f});
      
      if(!createBars(x, y, player, rend)) return false;
      
      m_manager->addComponent(player, Animator{
        .cols = 4,
        .rows = 1,
        .startFrame = 0,
        .frameCnt = 4,
        .frameTime = 0.15f
      });
      
      
      auto auraGem = m_manager->createEntity();
      m_manager->addComponent(auraGem, ActiveSkillGem{
        .skillIdHash = Hash("aura"),
        .lvl = 1
      });
      m_manager->addComponent(auraGem, InventoryItem{
        .owner = player,
        .isEquipped = true
      });
      m_manager->addComponent(auraGem, DirtyStatsTag{});
      
      return true;
    }
    bool createMobs(float x, float y, mip::IRenderer* rend, const std::string& txtrPath) {
      auto mob = m_manager->createEntity();
      m_manager->addComponent(mob, Kinematics{
        .z = 1,
        .pos = {x, y},
        .scale = {50, 50},
        .rot = 0.f
      });
      m_manager->addComponent(mob, Script{.task = squarePatrol(*m_manager, mob, 2.f, 3.f, 150.f)});
      
      auto texHandle = m_manager->loadAsset<std::shared_ptr<mip::ITexture>>(txtrPath);
      auto mobMat = rend->createMaterial("../../assets/shaders/shader.spv");
      if(!mobMat) {
        Logger::error("Failed to create material for mob!");
        return false;
      }
      if (auto tex = m_manager->getAsset(texHandle)) {
        mobMat->setTexture(0, *tex);
      }
      m_manager->addComponent(mob, Sprite{
        .mesh = rend->getGlobalQuad(),
        .material = mobMat
      });
      
      return true;
    }
    
  };
  
}; //game