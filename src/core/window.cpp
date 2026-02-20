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
      self->m_width = width;
      self->m_height = height;
      // glViewport(0, 0, width, height);
      Logger::info("Window resized to {}x{}", width, height);
    }
  }

  Window::Window(RendererType rType, int width, int height, const std::string& title, bool fullscreen)
    : m_width(width), m_height(height), isFullscreen(fullscreen) {
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
      m_width = mode->width;
      m_height = mode->height;
    }
    if(isFullscreen) glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    
    m_window = glfwCreateWindow(m_width, m_height, title.c_str(), NULL, NULL);
    
    if (!m_window) {
      glfwTerminate();
      Logger::error("Failed to create GLFW window!");
      return;
    }

    glfwSetWindowUserPointer(m_window, this);
  }

  Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
  }

  bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
  }

  float Window::getAspectRatio() const {
    if (m_height == 0) return 1.0f;
    return static_cast<float>(m_width) / static_cast<float>(m_height);
  }
  
  void Window::pollEvents() {
    glfwPollEvents();
  }
  
} //mip