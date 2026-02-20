#pragma once

#include "window.hpp"
#include "../graphics/i_renderer.hpp"
#include "../game/scene.hpp"
#include <chrono>

namespace mip {
	class Application {
  public:
    Application();
    ~Application();

    bool init();
    void run();

  private:
    
    std::unique_ptr<Window> m_window = nullptr;
    std::unique_ptr<IRenderer> m_renderer = nullptr;
    
    std::unique_ptr<game::Scene> m_scene = nullptr;
    
    void processInput(GLFWwindow* wnd, const float dT);
    
  };

}; //mip