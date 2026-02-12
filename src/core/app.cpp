#include "app.hpp"
#include "../common/logger.hpp"
#include "../graphics/renderer/vulkan/vk_renderer.hpp"
#include "../game/components.hpp"
#include "../game/loaders.hpp"
#include "../game/systems.hpp"

namespace mip {
  
	Application::Application() {
    Logger::info("Application created");
  }

  Application::~Application() {
    Logger::info("Application shutting down.");
  }

  
  bool Application::init() {
    Logger::info("Application initializing...");
    
    // renderer==================================================
    m_renderer = std::make_unique<VulkanRenderer>();
    m_renderer->setType(RendererType::VK);
    
    // WINDOW==================================================
    m_Window = std::make_unique<Window>(m_renderer->getType(), 1280, 720, "Magic is Power", true);
    // WINDOW==================================================
    
    if(!m_renderer->init(*m_Window)) {
      return false;
    }
    // renderer==================================================
    
    // scene==================================================
    m_mang = std::make_unique<ecs::Manager>();
    
    // 1. Components
    m_mang->registerComponent<game::Transform>();
    m_mang->registerComponent<game::Sprite>();
    m_mang->registerComponent<game::Velocity>();
    m_mang->registerComponent<game::PlayerTag>();
    
    // 2. Loaders + Assets
    game::TextureLoader tLoader{m_renderer.get()};
    m_mang->registerAsset<std::shared_ptr<ITexture>>(tLoader);
    
    // 3. Systems
    m_mang->registerSystem<game::PlayerControllerSystem>(m_Window->getWindow());
    m_mang->registerSystem<game::MovementSystem>();
    m_mang->registerSystem<game::RenderSystem>(m_renderer.get());
    
    // 4. Entities
    auto map = m_mang->createEntity();
    m_mang->addComponent(map, game::Transform{
      .pos = {m_Window->m_Width / 2, m_Window->m_Height / 2},
      .scale = {m_Window->m_Width, m_Window->m_Height},
      .rot = 0.f,
      .z = 0
    });
    auto texHandle = m_mang->loadAsset<std::shared_ptr<ITexture>>("../../assets/textures/1.png");
    auto mapMat = m_renderer->createMaterial("../../assets/shaders/shader.spv");
    if(!mapMat) {
      Logger::error("Failed to create material for sprite!");
      return false;
    }
    if (auto tex = m_mang->getAsset(texHandle)) {
      mapMat->setTexture(0, *tex);
    }
    m_mang->addComponent(map, game::Sprite{
      .mesh = m_renderer->getGlobalQuad(),
      .material = mapMat
    });
    
    auto player = m_mang->createEntity();
    m_mang->addComponent(player, game::PlayerTag{});
    m_mang->addComponent(player, game::Velocity{});
    m_mang->addComponent(player, game::Transform{
      .pos = {m_Window->m_Width / 2, m_Window->m_Height / 2},
      .scale = {250, 250},
      .rot = 0.f,
      .z = 1
    });
    texHandle = m_mang->loadAsset<std::shared_ptr<ITexture>>("../../assets/textures/txtr.jpg");
    auto playerMat = m_renderer->createMaterial("../../assets/shaders/shader.spv");
    if(!playerMat) {
      Logger::error("Failed to create material for sprite!");
      return false;
    }
    if (auto tex = m_mang->getAsset(texHandle)) {
      playerMat->setTexture(0, *tex);
    }
    m_mang->addComponent(player, game::Sprite{
      .mesh = m_renderer->getGlobalQuad(),
      .material = playerMat
    });
    // scene==================================================
    
    Logger::info("Application initialized successfully");
    return true;
  }
  
  
  void Application::run() {
    Logger::info("Entering main loop...");
    float lastFrameTime = 0.0f;

    while (!m_Window->shouldClose()) {
      float currentFrameTime = static_cast<float>(glfwGetTime());
      
      float deltaTime = currentFrameTime - lastFrameTime;
      lastFrameTime = currentFrameTime;
      
      processInput(m_Window->getWindow(), deltaTime);
      
      CameraInfo camData{};
      camData.pos = glm::vec3(0.f);
      camData.view = glm::mat4(1.f);
      camData.projection = glm::ortho(0.f, static_cast<float>(m_Window->m_Width), 0.f, static_cast<float>(m_Window->m_Height), -1.f, 1.f);
      
      if(!m_renderer->beginFrame(camData)) break;
      
      m_mang->update(deltaTime);
      
      if(!m_renderer->endFrame()) break;
      
      m_Window->pollEvents();
    }
    
  }
  
  void Application::processInput(GLFWwindow * wnd, const float dT) {
    static const float cSpd = 0.1f;
    static const float lSpd = 0.2f;
    
		if(glfwGetKey(wnd, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(wnd, true);
		}
    
	}
  
}; //mip