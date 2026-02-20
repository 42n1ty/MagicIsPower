#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace mip {
  
  enum class RendererType : uint16_t;
  
  class Window {
  public:
    Window(RendererType rType, int width, int height, const std::string& title, bool fullscreen);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void setShouldClose(bool flag) {glfwSetWindowShouldClose(m_window, flag);}
    GLFWwindow* getWindow() const { return m_window; }
    
    void pollEvents();

    float getAspectRatio() const;
    int m_width;
    int m_height;

  private:
    GLFWwindow* m_window;
    bool isFullscreen;

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void error_callback(int error, const char* description);
  };
  
}; //mip