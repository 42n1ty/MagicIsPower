#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "../common/ecs_core.hpp"
#include <../common/chunked_ma.hpp>
#include "skills_db.hpp"
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
          if(!clr) continue;
          
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
      
      auto validEs = rendQ | std::views::filter([&](RenderItem& item) {
        auto* act = manager.getComponent<Active>(item.e);
        return (!act || act->value);
      });
      
      for(const auto& item : validEs) {
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
      auto validAnchors  = anchors.getOwners() | std::views::filter([&](ecs::EntID ae) {
        auto* kin = ks.get(ae);
        auto* anch = anchors.get(ae);
        return (kin && anch);
      });
      // for(auto ae : anchors.getOwners()) {
      for(auto ae : validAnchors) {
        auto* kin = ks.get(ae);
        auto* anch = anchors.get(ae);
        // if(!kin || ! anch) continue;
        
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
          default: std::unreachable();
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
          default: std::unreachable();
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
      
      auto validBars = bars.getOwners() | std::views::filter([&](ecs::EntID be) {
        auto* kin = ks.get(be);
        auto* bar = bars.get(be);
        return kin && bar;
      });
      // for(auto be : bars.getOwners()) {
      for(auto be : validBars) {
        auto* kin = ks.get(be);
        auto* bar = bars.get(be);
        // if(!kin || !bar) continue;
        
        float percent;
        switch(bar->bType) {
          case BarType::HP: {
            percent = std::max(ph->cur / ph->max, 0.f);
            break;
          }
          case BarType::EXP: {
            percent = std::max((float)pexp->cur / (float)pexp->max, 0.f);
            break;
          }
          default: std::unreachable();
        }
        kin->scale.x = bar->maxW * percent;
      }
    }
  };
  
  class GamePlayUISystem : public ecs::ISystem {
    GLFWwindow* m_wnd = nullptr;
  public:
    
    GamePlayUISystem(GLFWwindow* wnd) : m_wnd(wnd) {}
    
    void update(ecs::Manager& manager, const float dT) override {
      ecs::EntID pe = ecs::NULL_ENT;
      Exp* exp = nullptr;
      GameState* state = nullptr;
      Health* health = nullptr;
      ActiveSkillGem* aura = nullptr;
      uint32_t count = 0;
      
      for(auto e : manager.view<PlayerTag>().getOwners()) {
        pe = e;
        exp = manager.getComponent<Exp>(pe);
        state = manager.getComponent<GameState>(pe);
        health = manager.getComponent<Health>(pe);
        if(!exp || !state)
          return;
        break;
      }
      
      for(auto ge : manager.view<ActiveSkillGem>().getOwners()) {
        auto* item = manager.getComponent<InventoryItem>(ge);
        if(item && item->owner == pe && item->isEquipped) {
          aura = manager.getComponent<ActiveSkillGem>(ge);
        }
      }
      for(auto ie : manager.view<InventoryItem>().getOwners()) {
        auto* item = manager.getComponent<InventoryItem>(ie);
        if(item && item->owner == pe && item->isEquipped == false) count++;
      }
      
      //lvl up logic
      if(exp->cur >= exp->max && !state->isLvlUp) {
        exp->cur -= exp->max;
        exp->max = static_cast<uint32_t>(exp->max * 1.2f);
        exp->curLvl++;
        health->cur = health->max;
        
        state->isLvlUp = true;
        state->isPaused = true;
      }
      
      //hud
      ImGui::SetNextWindowPos(ImVec2(10, 10));
      ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | /*ImGuiWindowFlags_NoBackground | */ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Level: %d", exp->curLvl);
      ImGui::Text("EXP: %d / %d", (int)exp->cur, (int)exp->max);
      ImGui::Text("Aura lvl: %d", (int)aura->lvl);
      ImGui::Text("Aura curLvl dmg: %d", (int)aura->curLvlDmg);
      ImGui::Text("Aura final dmg: %d", (int)aura->finalDmg);
      ImGui::Text("Items picked up: %d", (int)count);
      ImGui::End();
      
      //lvl up rendering
      if(state->isLvlUp) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 400));
        
        ImGui::Begin("LEVEL UP!!!", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        
        ImGui::Text("Choose upgrade:");
        ImGui::Separator();
        
        //1st card
        ImGui::BeginChild("Card1", ImVec2(180, 300), true);
        ImGui::Text("AoE range 20%%");
        ImGui::TextWrapped("Increases area of effect for all skills");
        ImGui::SetCursorPosY(260);
        if(ImGui::Button("Select##1", ImVec2(160, 30))) {
          if(auto* pStats = manager.getComponent<PermanentStats>(pe)) {
            pStats->incAoERadius += 0.2f;
            manager.addComponent(pe, DirtyStatsTag{});
          }
          
          state->isLvlUp = false;
          state->isPaused = false;
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        ImGui::BeginChild("Card2", ImVec2(180, 300), true);
        ImGui::Text("Aura level up");
        ImGui::TextWrapped("Increase base aura damage");
        ImGui::SetCursorPosY(260);
        if(ImGui::Button("Select##2", ImVec2(160, 30))) {
          for(auto ge : manager.view<ActiveSkillGem>().getOwners()) {
            auto* gem = manager.getComponent<ActiveSkillGem>(ge);
            auto* item = manager.getComponent<InventoryItem>(ge);
            
            if(item && item->owner == pe && gem->skillIdHash == Hash("aura")) {
              gem->lvl++;
              manager.addComponent(pe, DirtyStatsTag{});
              break;
            }
          }
          
          state->isLvlUp = false;
          state->isPaused = false;
        }
        ImGui::EndChild();
        
        ImGui::End();
      }
      
      // loot pick up
      int w, h;
      glfwGetWindowSize(m_wnd, &w, &h);
      float halfW = w / 2.f;
      float halfH = h / 2.f;
      
      glm::vec2 playerPos = manager.getComponent<Kinematics>(pe)->pos;
      
      ImDrawList* drawList = ImGui::GetBackgroundDrawList();
      ImVec2 mousePos = ImGui::GetMousePos();
      bool clicked = ImGui::IsMouseClicked(0);
      
      std::vector<ecs::EntID> pickedUpItems;
      
      for(auto ie : manager.view<GroundItem>().getOwners()) {
        auto* item = manager.getComponent<GroundItem>(ie);
        auto* kin = manager.getComponent<Kinematics>(ie);
        if(!ie || !kin) continue;
        
        float screenX = kin->pos.x - playerPos.x + halfW;
        float screenY = kin->pos.y - playerPos.y + halfH;
        
        if(screenX < -100.f || screenX > w + 100.f || screenY < -100.f || screenY > h + 100.f) continue;
        
        ImVec2 textSize = ImGui::CalcTextSize(item->name.c_str());
        ImVec2 textPos(screenX - textSize.x / 2.f, screenY - 20.f);
        
        ImVec2 rectMin(textPos.x - 5.f, textPos.y - 2.f);
        ImVec2 rectMax(textPos.x + textSize.x + 5.f, textPos.y + textSize.y + 2.f);
        
        bool hovered = mousePos.x >= rectMin.x && mousePos.x <= rectMax.x && mousePos.y >= rectMin.y && mousePos.y <= rectMax.y;
        
        ImColor bgClr = hovered ? ImColor(80, 80, 80, 200) : ImColor(0, 0, 0, 150);
        drawList->AddRectFilled(rectMin, rectMax, bgClr, 3.f);
        
        ImColor textClr = (item->rarity == ItemRarity::Epic) ? ImColor(255, 150, 0) : ImColor(255, 255, 255);
        drawList->AddText(textPos, textClr, item->name.c_str());
        
        if(hovered && clicked) {
          pickedUpItems.emplace_back(ie);
        }
      }
      
      for(auto ie : pickedUpItems) {
        manager.addComponent(ie, InventoryItem{.owner = pe, .isEquipped = false});
        manager.removeComponent<GroundItem>(ie);
        manager.removeComponent<Kinematics>(ie);
        manager.removeComponent<Sprite>(ie);
      }
    }
  };
  
  struct DeathEvent {
    glm::vec2 pos;
    uint32_t enemyIdHash;
    DeathEvent* next;
  };
  class LootSystem : public ecs::ISystem {
    mip::IRenderer* m_rend;
    ChunkedMA* m_arena;
    std::shared_ptr<mip::IMaterial> m_lootMat;
    
  public:
    static inline DeathEvent* frameDeaths = nullptr;
    
    LootSystem(mip::IRenderer* rend, ChunkedMA* arena) : m_rend(rend), m_arena(arena) {
      m_lootMat = m_rend->createMaterial("../../assets/shaders/shader.spv");
      auto tex = m_rend->createTexture("../../assets/textures/whitepixel.png", false);
      if(tex) m_lootMat->setTexture(0, tex);
    }
    
    void update(ecs::Manager& manager, const float dT) override {
      ecs::EntID pe = ecs::NULL_ENT;
      PlayerLootFilter* filter = nullptr;
      Materials* mats = nullptr;
      
      for(auto e : manager.view<PlayerTag>().getOwners()) {
        pe = e;
        filter = manager.getComponent<PlayerLootFilter>(e);
        mats = manager.getComponent<Materials>(e);
        break;
      }
      
      DeathEvent* de = frameDeaths;
      while(de != nullptr) {
        
        bool isGemDrop = (rand() % 100) < 50;
        
        ItemRarity rolledRarity = isGemDrop ?  ItemRarity::Epic : ItemRarity::Junk;
        ItemCategory rolledCategory = isGemDrop ?  ItemCategory::SkillGem : ItemCategory::Weapon;
        std::string rolledID = isGemDrop ? "gem" : "rusty_sword";
        
        FilterAction action = FilterAction::Show;
        if(filter) {
          action = filter->evaluate(rolledRarity, rolledCategory);
        }
        
        if(action == FilterAction::AutoSalvage) {
          if(mats) mats->scrapMetal += 1;
          Logger::debug("Junk dropped: {}", mats->scrapMetal);
        }
        else if(action == FilterAction::Show) {
          ecs::EntID itemE = manager.createEntity();
          manager.addComponent(itemE, Active{});
          manager.addComponent(itemE, GroundItem{.name = rolledID, .rarity = rolledRarity});
          manager.addComponent(itemE, Kinematics{.z = 5, .pos = de->pos, .scale = {30.f, 30.f}});
          glm::vec4 color = (rolledRarity == ItemRarity::Epic) ?  glm::vec4(1.f, 0.5f, 0.f, 1.f) : glm::vec4(0.8f, 0.8f, 0.8f, 1.f);
          manager.addComponent(itemE, ColorTint{.baseColor = color});
          manager.addComponent(itemE, Sprite{.mesh = m_rend->getGlobalQuad(), .material = m_lootMat});
          Logger::debug("Gem dropped: {}", rolledID);
        }
        
        de = de->next;
      }
      
      frameDeaths = nullptr;
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
  
  class AnimSystem : public ecs::ISystem {
  public:
    void update(ecs::Manager& manager, const float dT) override {
      
      //check game state
      for (auto e : manager.view<game::PlayerTag>().getOwners()) {
        if (auto* state = manager.getComponent<game::GameState>(e)) {
          if (state->isPaused) return;
        }
        break;
      }
      
      auto& animators = manager.view<Animator>();
      auto& sprites = manager.view<Sprite>();
      auto& acts = manager.view<Active>();
      
      auto validAnims = animators.getOwners() | std::views::filter([&](ecs::EntID ae) {
        auto* act = acts.get(ae);
        auto* spr = sprites.get(ae);
        return ((!act || act->value) && (spr));
      });
      // for(auto ae : animators.getOwners()) {
      for(auto ae : validAnims) {
        // if(act && !act->value) continue;
        
        auto* anim = animators.get(ae);
        auto* spr = sprites.get(ae);
        
        // if(!spr) continue;
        
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
      
      //check game state
      for (auto e : manager.view<game::PlayerTag>().getOwners()) {
        if (auto* state = manager.getComponent<game::GameState>(e)) {
          if (state->isPaused) return;
        }
        break;
      }
      
      auto& ks = manager.view<Kinematics>();
      
      const auto& kinOwners = ks.getOwners();
      auto& kinDense = ks.getDense();
      
      for(size_t i = 0; i < kinDense.size(); ++i) {
        ecs::EntID e = kinOwners[i];
        auto* active = manager.getComponent<Active>(e);
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
      
      //check game state
      for (auto e : manager.view<game::PlayerTag>().getOwners()) {
        if (auto* state = manager.getComponent<game::GameState>(e)) {
          if (state->isPaused) return;
        }
        break;
      }
      
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
      
      //check game state
      for (auto e : manager.view<game::PlayerTag>().getOwners()) {
        if (auto* state = manager.getComponent<game::GameState>(e)) {
          if (state->isPaused) return;
        }
        break;
      }
      
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
  
  class EnemySpawnerSystem : public ecs::ISystem {
    mip::IRenderer* m_rend;
    std::vector<ecs::EntID> m_pool;
    Task m_waveTask{nullptr};
    float m_waveTimer = 0.f;
    float m_spawnRadius = 800.f;
    std::shared_ptr<mip::IMaterial> m_enemyMat;
    ChunkedMA* m_arena;
    
    float m_speed; //temp need enemyConfigs later
    
  public:
    
    inline static uint32_t diedCount = 0;
    
    EnemySpawnerSystem(mip::IRenderer* rend, ChunkedMA* arena) : m_rend(rend) {
      m_enemyMat = m_rend->createMaterial("../../assets/shaders/shader.spv");
      auto tex = m_rend->createTexture("../../assets/textures/mob1.png", false);
      m_enemyMat->setTexture(0, tex);
      
      m_arena = arena;
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
          .z = 9,
          .pos = {spawnPos.x, spawnPos.y},
          .scale = {50.f, 50.f},
          .rot = 0.f,
          .speed = m_speed
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
      }
    }
    
    void update(ecs::Manager& manager, const float dT) override {
      
      ecs::EntID pe = ecs::NULL_ENT;
      
      //check game state
      for (auto e : manager.view<game::PlayerTag>().getOwners()) {
        pe = e;
        if (auto* state = manager.getComponent<game::GameState>(e)) {
          if (state->isPaused) return;
        }
        break;
      }
      
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
      plPos = manager.getComponent<Kinematics>(pe)->pos;
      
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
      auto* pexp = manager.getComponent<Exp>(pe);
      auto& healths = manager.view<Health>();
      for (auto e : manager.view<EnemyTag>().getOwners()) {
        auto* active = manager.getComponent<Active>(e);
        if (active->value && healths.get(e)->cur <= 0) {
          active->value = false;
          pexp->cur += 1;
          Logger::debug("Enemy died! #{}", diedCount++);
          m_pool.push_back(e);
          
          auto* kin = manager.getComponent<Kinematics>(e);
          if(kin) {
            auto* de = m_arena->alloc<DeathEvent>();
            de->pos = kin->pos;
            de->next = LootSystem::frameDeaths;
            LootSystem::frameDeaths = de;
          }
          // exp
        }
      }
    }
    
  };
  
  class DamageSystem : public ecs::ISystem {
    
    struct GridNode {
      ecs::EntID e;
      GridNode* next;
    };
    
    static const int HASH_SIZE = 4096;
    GridNode* m_hashTable[HASH_SIZE];
    ChunkedMA* m_arena{nullptr};
    
    std::vector<ecs::EntID> m_toDestroy;
    
    inline int getHash(int cx, int cy) {
      uint32_t hash = (static_cast<uint32_t>(cx) * 73856093u) ^ (static_cast<uint32_t>(cy) * 19349663u);
      return hash % HASH_SIZE;
    }
    
  public:
    
    DamageSystem(ChunkedMA* arena)
    : m_arena((arena)) {
      m_toDestroy.reserve(100);
    }
    
    void update(ecs::Manager& manager, const float dT) override {
      
      //check game state
      for (auto e : manager.view<game::PlayerTag>().getOwners()) {
        if (auto* state = manager.getComponent<game::GameState>(e)) {
          if (state->isPaused) return;
        }
        break;
      }
      
      m_toDestroy.clear();
      
      auto& healths = manager.view<Health>();
      auto& enemies = manager.view<EnemyTag>();
      auto& circles = manager.view<CircleCollider>();
      auto& ks = manager.view<Kinematics>();
      auto& dds = manager.view<DamageDealer>();
      auto& acts = manager.view<Active>();
      auto& pulses = manager.view<PulseCooldown>();
      auto& players = manager.view<PlayerTag>();
      
      memset(m_hashTable, 0, sizeof(m_hashTable));
      
      const float CELLSIZE = 100.f; //need to be more than the biggest enemy radius
      
      for(auto ee : enemies.getOwners()) {
        auto* eact = acts.get(ee);
        if(eact && !eact->value) continue;
        
        auto* ek = ks.get(ee);
        if(!ek) continue;
        
        int cx = static_cast<int>(std::floorf(ek->pos.x / CELLSIZE));
        int cy = static_cast<int>(std::floorf(ek->pos.y / CELLSIZE));
        
        int hash = getHash(cx, cy);
        
        GridNode* node = m_arena->alloc<GridNode>();
        node->e = ee;
        node->next = m_hashTable[hash];
        m_hashTable[hash] = node;
      }
      
      // 1st iter by weapons
      for(auto we : dds.getOwners()) {
        auto* wact = acts.get(we);
        if(wact && !wact->value) continue;
        
        auto* pulse = pulses.get(we);
        if(pulse && pulse->curTimer > 0.f) continue;
        
        auto* wc = circles.get(we);
        auto* wk = ks.get(we);
        auto* dmg = dds.get(we);
        if(!wc || !wk || !dmg) continue;
        
        // cell coords of dd
        int minx = static_cast<int>(std::floorf((wk->pos.x - wc->radius) / CELLSIZE));
        int maxx = static_cast<int>(std::floorf((wk->pos.x + wc->radius) / CELLSIZE));
        int miny = static_cast<int>(std::floorf((wk->pos.y - wc->radius) / CELLSIZE));
        int maxy = static_cast<int>(std::floorf((wk->pos.y + wc->radius) / CELLSIZE));
        
        // 2nd iter by enemies
        auto checkCollisions = [&](){
          
          for(int x = minx; x <= maxx; ++x) {
            for(int y = miny; y <= maxy; ++y) {
              
              int hash = getHash(x, y);
              GridNode* node = m_hashTable[hash];
              
              while(node != nullptr) {
                ecs::EntID ee = node->e;
                GridNode* nextNode = node->next;
                
                auto* eact = acts.get(ee);
                auto* ehp = healths.get(ee);
                if((eact && !eact->value) || (ehp && ehp->cur <= 0.f)) {
                  node = nextNode;
                  continue;
                }
                
                auto* ec = circles.get(ee);
                auto* ek = ks.get(ee);
                if(!ec || !ek) {
                  node = nextNode;
                  continue;
                }
                
                float finalDmg = dmg->amount;
                // if(auto* res = manager.getComponent<Resistances>(ee)) {
                //   float targetRes = 0.f;
                //   if(dmg->dmgType == SkillTag::Fire) targetRes = res->fire;
                //   else if(dmg->dmgType == SkillTag::Water) targetRes = res->water;
                //   else if(dmg->dmgType == SkillTag::Air) targetRes = res->air;
                //   else if(dmg->dmgType == SkillTag::Earth) targetRes = res->earth;
                //   else if(dmg->dmgType == SkillTag::Cold) targetRes = res->cold;
                //   else if(dmg->dmgType == SkillTag::Lightning) targetRes = res->lightning;
                  
                //   targetRes -= dmg->pen;
                //   targetRes = std::max(targetRes, 2.f);
                  
                //   finalDmg *= (1.f - targetRes);
                // }
                
                float dist = glm::distance(wk->pos, ek->pos);
                if(dist < (wc->radius + ec->radius)) {
                  ehp->cur -= finalDmg; // once damage
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
                      return;
                    }
                  }
                }
                
                node = nextNode;
              }
            }
          }
        };
        
        checkCollisions();
        
        if(pulse) pulse->curTimer = pulse->maxTimer;
      }
      
      //player & enemy colls
      for(auto pe : players.getOwners()) {
        auto* ph = healths.get(pe);
        auto* pt = ks.get(pe);
        auto* pc = circles.get(pe);
        for(auto ee : enemies.getOwners()) {
          auto* eact = acts.get(ee);
          auto* ehp = healths.get(ee);
          if((eact && !eact->value) || (ehp && ehp->cur <= 0.f)) continue;
          
          auto* ec = circles.get(ee);
          auto* ek = ks.get(ee);
          if(!ec || !ek) continue;
        
          float dist = glm::distance(pt->pos, ek->pos);
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
  
  class StatCalcSystem : public ecs::ISystem {
    SkillDB* m_skillDB;
  public:
    StatCalcSystem(SkillDB* db) : m_skillDB(db) {}
    
    void update(ecs::Manager& manager, const float dT) override {
      for(auto pe : manager.view<PlayerTag>().getOwners()) {
        if(manager.getComponent<DirtyStatsTag>(pe)) {
          auto* pStats = manager.getComponent<PlayerStats>(pe);
          auto* prStats = manager.getComponent<PermanentStats>(pe);
          if(!pStats) continue;
          
          pStats->incFireDmg = 0.f;
          pStats->incColdDmg = 0.f;
          pStats->incAoERadius = 0.f;
          pStats->cdReduction = 0.f;
          pStats->extraProj = 0;
          
          //TODO: gear cycle
          
          pStats->incFireDmg = prStats->incFireDmg;
          pStats->incColdDmg = prStats->incColdDmg;
          pStats->incAoERadius = prStats->incAoERadius;
          pStats->cdReduction = prStats->cdReduction;
          pStats->extraProj = prStats->extraProj;
          
          manager.removeComponent<DirtyStatsTag>(pe);
          
          for(auto ge : manager.view<ActiveSkillGem>().getOwners()) {
            auto* item = manager.getComponent<InventoryItem>(ge);
            if(item && item->owner == pe && item->isEquipped) {
              manager.addComponent(ge, DirtyStatsTag{});
            }
          }
        }
      }
      
      auto& dirtyGems = manager.view<DirtyStatsTag>();
      for(int i = static_cast<int>(dirtyGems.getDense().size()) - 1; i >= 0; --i) {
        ecs::EntID ge = dirtyGems.getOwners()[i];
        auto* gem = manager.getComponent<ActiveSkillGem>(ge);
        auto* item = manager.getComponent<InventoryItem>(ge);
        if(!gem || !item || !m_skillDB->activeSkills.count(gem->skillIdHash)) continue;
        
        auto* pStats = manager.getComponent<PlayerStats>(item->owner);
        const auto& baseConf = m_skillDB->activeSkills[gem->skillIdHash];
        
        gem->tagsMask = baseConf.tagsMask;
        gem->finalDmg = baseConf.baseDmg * (1.f + (gem->lvl * 0.5f));
        gem->curLvlDmg = gem->finalDmg;
        gem->finalCd = baseConf.baseCd;
        gem->finalRadius = baseConf.baseRadius;
        gem->finalProj = baseConf.baseProj;
        gem->dmgMultiplier = 1.f;
        
        float totalIncDmg = 0.f;
        float totalIncRadius = 0.f;
        
        //TODO: count passives
        
        if(auto* links = manager.getComponent<LinkedGems>(ge)) {
          for(auto se : links->gems) {
            if(auto* sg = manager.getComponent<SupGem>(se)) {
              if(m_skillDB->supSkills.count(sg->supIdHash)) {
                m_skillDB->supSkills[sg->supIdHash].applyMods(*gem, sg->lvl);
              }
            }
          }
        }
        
        if(gem->tagsMask & SkillTag::Fire) {
          totalIncDmg += pStats->incFireDmg;
        }
        if(gem->tagsMask & SkillTag::Cold) {
          totalIncDmg += pStats->incColdDmg;
        }
        if(gem->tagsMask & SkillTag::AoE) {
          totalIncRadius += pStats->incAoERadius;
        }
        //TODO: other mods
        
        gem->finalDmg = gem->finalDmg * (1.f + totalIncDmg) * gem->dmgMultiplier;
        gem->finalRadius = gem->finalRadius * (1.f + totalIncRadius);
        
        if (gem->spawnedEnt != ecs::NULL_ENT) {
          if (auto* dmg = manager.getComponent<DamageDealer>(gem->spawnedEnt)) {
            dmg->amount = gem->finalDmg;
          }
          if (auto* col = manager.getComponent<CircleCollider>(gem->spawnedEnt)) {
            col->radius = gem->finalRadius;
          }
          if (auto* kin = manager.getComponent<Kinematics>(gem->spawnedEnt)) {
            kin->scale = {gem->finalRadius * 2.f, gem->finalRadius * 2.f};
          }
          if (auto* pulse = manager.getComponent<PulseCooldown>(gem->spawnedEnt)) {
            pulse->maxTimer = gem->finalCd;
          }
        }
        
        manager.removeComponent<DirtyStatsTag>(ge);
      }
    }
  };
  
  class CombatSystem : public ecs::ISystem {
    SkillDB* m_skillDB;
    GLFWwindow* m_wnd;
    
  public:
    CombatSystem(SkillDB* db, GLFWwindow* wnd) : m_skillDB(db), m_wnd(wnd) {}
    
    void update(ecs::Manager& manager, const float dT) override {
      ecs::EntID pe = ecs::NULL_ENT;
      glm::vec2 pos{0.f};
      for(auto e : manager.view<PlayerTag>().getOwners()) {
        pe = e;
        pos = manager.getComponent<Kinematics>(pe)->pos;
        break;
      }
      if(pe == ecs::NULL_ENT) return;
      
      //TODO: fire dir, cast etc
      bool isClicking = glfwGetMouseButton(m_wnd, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
      double x, y;
      glfwGetCursorPos(m_wnd, &x, &y);
      int w, h;
      glfwGetWindowSize(m_wnd, &w, &h);
      glm::vec2 playerPos{w / 2.f, h / 2.f};
      glm::vec2 mousePos{static_cast<float>(x), static_cast<float>(y)};
      glm::vec2 targetDir = mousePos - playerPos;
      if(glm::length(targetDir) > 0.1f) {
        targetDir = glm::normalize(targetDir);
      }
      else {
        targetDir = {1.f, 0.f};
      }
      
      for(auto ge :  manager.view<ActiveSkillGem>().getOwners()) {
        auto* gem = manager.getComponent<ActiveSkillGem>(ge);
        auto* item = manager.getComponent<InventoryItem>(ge);
        if(!item || !item->isEquipped || item->owner != pe) continue;
        if(!m_skillDB->activeSkills.count(gem->skillIdHash)) continue;
        
        const auto& config = m_skillDB->activeSkills[gem->skillIdHash];
        
        if(config.castType == CastType::Persistent && isClicking) {
          if(gem->spawnedEnt == ecs::NULL_ENT) {
            gem->spawnedEnt = config.buildPrefub(manager, pos, {0.f, 0.f}, *gem);
            manager.addComponent(gem->spawnedEnt, AttachTo{.target = pe});
          }
          else {
            if(auto* act = manager.getComponent<Active>(gem->spawnedEnt)) act->value = !act->value;
          }
          continue;
        }
        
        if(config.castType == CastType::Continuous) {
          if(gem->curCdTimer > 0.f) gem->curCdTimer -= dT;
          
          if(isClicking && gem->curCdTimer <= 0.f) {
            for(int i = 0; i < gem->finalProj; ++i) {
              glm::vec2 vel = targetDir * 300.f;
              config.buildPrefub(manager, pos, vel, *gem);
            }
            gem->curCdTimer = gem->finalCd;
          }
        }
      }
    }
  };
  
  class StatusSystem : public ecs::ISystem {
  public:
    void update(ecs::Manager& manager, const float dT) override {
      
      //check game state
      for (auto e : manager.view<game::PlayerTag>().getOwners()) {
        if (auto* state = manager.getComponent<game::GameState>(e)) {
          if (state->isPaused) return;
        }
        break;
      }
      
      auto& healths = manager.view<Health>();
      auto& statuses = manager.view<StatusEffects>();
      auto& acts = manager.view<Active>();
      
      for(auto se : statuses.getOwners()) {
        auto* act = acts.get(se);
        if(act && !act->value) continue;
        
        auto* hp = healths.get(se);
        auto* status = statuses.get(se);
        
        if(!hp || !status) continue;
        
        std::erase_if(status->dots, [&](DoTCharge& dot) {
          dot.lifetime -= dT;
          dot.curTickTimer -= dT;
          
          if(dot.curTickTimer <= 0.f) {
            hp->cur -= dot.damage;
            dot.curTickTimer = dot.tickRate;
          }
          
          return dot.lifetime <= 0.f;
        });
        
      }
    }
  };
  
  class AttachmentSystem : public ecs::ISystem {
    void update(ecs::Manager& manager, const float dT) override {
      
      //check game state
      for (auto e : manager.view<game::PlayerTag>().getOwners()) {
        if (auto* state = manager.getComponent<game::GameState>(e)) {
          if (state->isPaused) return;
        }
        break;
      }
      
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
      
      //check game state
      for (auto e : manager.view<game::PlayerTag>().getOwners()) {
        if (auto* state = manager.getComponent<game::GameState>(e)) {
          if (state->isPaused) return;
        }
        break;
      }
      
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