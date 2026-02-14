#include "app.hpp"
#include "../common/logger.hpp"
#include "../graphics/renderer/vulkan/vk_renderer.hpp"

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
    m_scene = std::make_unique<game::Scene>();
    if(!m_scene->init(m_Window->getWindow(), m_renderer.get())) return false;
    
    if(
         !m_scene->createLevel(m_Window->m_Width, m_Window->m_Height, m_renderer.get(), "../../assets/textures/map.png")
      || !m_scene->createPlayer(m_Window->m_Width / 3, m_Window->m_Height / 3, m_renderer.get(), "../../assets/textures/player1.png")
      || !m_scene->createMobs(m_Window->m_Width / 3, m_Window->m_Height / 3, m_renderer.get(), "../../assets/textures/mob1.png")
      
    ) return false;   
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
      
      m_scene->update(deltaTime);
      
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