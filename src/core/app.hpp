#pragma once

#include "window.hpp"
#include "../graphics/i_renderer.hpp"
#include "../graphics/sprite.hpp"
#include <chrono>

namespace mip {
	class Application {
  public:
    Application();
    ~Application();

    bool init();
    void run();

  private:
    
    //systems first
    std::unique_ptr<Window> m_Window = nullptr;
    std::unique_ptr<IRenderer> m_renderer = nullptr;
    
    //and then resources
    // std::unique_ptr<Model> m_model{nullptr};
    std::unique_ptr<Sprite> m_sprite = nullptr;
    
    void processInput(GLFWwindow* wnd, const float dT);
    
  };

}; //mip