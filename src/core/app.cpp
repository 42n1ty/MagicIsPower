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
    // m_renderer = std::make_unique<VulkanRenderer>();
    // m_renderer->setType(RendererType::VK);
    m_renderer = std::make_unique<VulkanRenderer>();
    m_renderer->setType(RendererType::VK);
    
    // WINDOW==================================================
    m_Window = std::make_unique<Window>(m_renderer->getType(), 1280, 720, "Magic is Power", false);
    // WINDOW==================================================
    
    if(!m_renderer->init(*m_Window)) {
      return false;
    }
    // renderer==================================================
    
    // model==================================================
    // m_model = std::make_unique<Model>(false); //this chest needn't flip vertically (I forgot why it was flipped in prev versions)
    // if(!m_model->load(m_renderer.get(), "../../assets/models/chest/source/MESH_Chest.fbx")) {
    //   Logger::error("Failed to load model");
    //   return false;
    // }
    // model==================================================
    
    // sprite==================================================
    m_sprite = std::make_unique<Sprite>();
    if(!m_sprite->init(m_renderer.get(), "../../assets/textures/1.png")) {
      Logger::error("Failed to init sprite");
      return false;
    }
    m_sprite->setPos({1280.f / 2.f, 720.f / 2.f});
    m_sprite->setScale({m_Window->m_Width, m_Window->m_Height});
    // sprite==================================================
    
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
      
      // m_model->updAnim(deltaTime);
      
      CameraInfo camData{};
      // glm::vec3 eyePos = glm::vec3(0.f, 2.f, 5.f);
      // glm::vec3 center = glm::vec3(0.f, 0.f, 0.f);
      // glm::vec3 upV = glm::vec3(0.f, 1.f, 0.f);
      camData.pos = glm::vec3(0.f);
      // camData.view = glm::lookAt(eyePos, center, upV);
      camData.view = glm::mat4(1.f);
      // camData.projection = glm::perspective(glm::radians(45.f), static_cast<float>(m_Window->m_Width) / static_cast<float>(m_Window->m_Height), 0.1f, 100.f);
      camData.projection = glm::ortho(0.f, static_cast<float>(m_Window->m_Width), 0.f, static_cast<float>(m_Window->m_Height), -1.f, 1.f);
      
      if(!m_renderer->beginFrame(camData)) break;
      
      // glm::mat4 modelTransform = glm::mat4(1.f);
      // modelTransform *= m_model->getNormMatrix();
      // if(!m_model->draw(m_renderer.get(), modelTransform)){
      //   // Logger::error("BREAK");
      //   break;
      // }
      
      m_sprite->draw(m_renderer.get());
      
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