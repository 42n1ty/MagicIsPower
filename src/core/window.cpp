#include "../common/logger.hpp"
#include "window.hpp"
#include "../graphics/i_renderer.hpp"

namespace mip {
  void Window::error_callback(int error, const char* description) {
    Logger::error("GLFW Error {}: {}", error, description);
  }

  void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self) {
      self->m_Width = width;
      self->m_Height = height;
      // glViewport(0, 0, width, height);
      Logger::info("Window resized to {}x{}", width, height);
    }
  }

  Window::Window(RendererType rType, int width, int height, const std::string& title, bool fullscreen)
    : m_Width(width), m_Height(height), isFullscreen(fullscreen) {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
      Logger::error("Failed to initialize GLFW!");
      return;
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if(isFullscreen) {
      GLFWmonitor* mon = glfwGetPrimaryMonitor();
      if(!mon) {
        Logger::error("Failed to find primary monitor");
        isFullscreen = false;
        return;
      }
      const GLFWvidmode* mode = glfwGetVideoMode(mon);
      if(!mode) {
        Logger::error("Failed to get video mode");
        isFullscreen = false;
        return;
      }
      m_Width = mode->width;
      m_Height = mode->height;
    }
    if(isFullscreen) glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    
    m_Window = glfwCreateWindow(m_Width, m_Height, title.c_str(), NULL, NULL);
    
    if (!m_Window) {
      glfwTerminate();
      Logger::error("Failed to create GLFW window!");
      return;
    }

    glfwSetWindowUserPointer(m_Window, this);
  }

  Window::~Window() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
  }

  bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_Window);
  }

  float Window::getAspectRatio() const {
    if (m_Height == 0) return 1.0f;
    return static_cast<float>(m_Width) / static_cast<float>(m_Height);
  }
  
  void Window::pollEvents() {
    glfwPollEvents();
  }
  
} //mip