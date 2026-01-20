#pragma once

#include "renderer/vulkan/vk_renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>


namespace mip {
  
  class Sprite {
  public:
    
    Sprite() = default;
    
    bool init(IRenderer* renderer, const std::string& txtrPath) {
      // m_mesh = renderer->createMesh(IMesh::createQuad()); //not optimized!!!
      m_mesh = renderer->getGlobalQuad();
      if(!m_mesh) {
        Logger::error("Failed to create mesh for sprite!");
        return false;
      }
      
      auto txtr = renderer->createTexture(txtrPath, false);
      if(!txtr) {
        Logger::error("Failed to create texture for sprite!");
        return false;
      }
      
      m_material = renderer->createMaterial("../../assets/shaders/shader.spv");
      if(!m_material) {
        Logger::error("Failed to create material for sprite!");
        return false;
      }
      
      m_material->setTexture(0, txtr);
      
      m_scale = glm::vec2(100.f, 100.f);
      m_pos = glm::vec2(0.f, 0.f);
      m_rotation = 0.f;
      
      return true;
    }
    
    void setPos(glm::vec2 pos) {m_pos = pos;}
    void setScale(glm::vec2 scale) {m_scale = scale;}
    void setRotation(float rotation) {m_rotation = rotation;}
    
    void draw(IRenderer* renderer) {
      glm::mat4 model = glm::mat4(1.f);
      model = glm::translate(model, glm::vec3(m_pos, 0.f));
      model = glm::rotate(model, glm::radians(m_rotation), glm::vec3(0.f, 0.f, 1.f));
      model = glm::scale(model, glm::vec3(m_scale, 1.f));
      
      renderer->submit(m_mesh, m_material, model);
    }
    
  private:
    
    std::shared_ptr<IMesh> m_mesh;
    std::shared_ptr<IMaterial> m_material;
    
    glm::vec2 m_pos;
    glm::vec2 m_scale;
    float m_rotation;
  };
  
  
  
}; //mip