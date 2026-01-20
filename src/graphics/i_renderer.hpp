#pragma once

#include <string>
#include <vector>

#include "i_mesh.hpp"
#include "i_texture.hpp"
#include "i_material.hpp"
#include "../common/logger.hpp"



namespace mip {
  
  class Window;
  
  enum class RendererType : uint16_t{
    OGL = 0,
    VK,
    SIZE
  };
  
  struct CameraInfo {
    glm::vec3 pos;
    glm::mat4 projection;
    glm::mat4 view;
  };
  
  /**
   * @brief Renderer Interface
   * Responsible for creating resources and managing the frame rendering cycle
   */
  class IRenderer {
  public:
    virtual ~IRenderer() = default;
    
    virtual bool init(Window& wnd) = 0;
    virtual void shutdown() = 0;
    
    std::shared_ptr<IMesh> getGlobalQuad() {return m_globQuad;} //for 2D optimization
    
    virtual std::shared_ptr<IMesh> createMesh(const MeshData& data) = 0; //from model
    virtual std::shared_ptr<ITexture> createTexture(const std::string& path, const bool flip) = 0; //from model
    virtual std::shared_ptr<IMaterial> createMaterial(const std::string& vertShaderPath, const std::string& fragShaderPath = "") = 0; //from model
    
    virtual bool beginFrame(const CameraInfo& camera) = 0;
    virtual bool submit(std::shared_ptr<IMesh> mesh, std::shared_ptr<IMaterial> material, const glm::mat4& transform) = 0;
    // virtual bool submitAnimated(std::shared_ptr<IMesh> mesh, std::shared_ptr<IMaterial> material, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms) = 0;
    virtual bool endFrame() = 0;
    
    RendererType getType() { return renderer_type; }
    void setType(RendererType type) { renderer_type = type; }
    
    std::shared_ptr<IMesh> m_globQuad{nullptr}; // for 2D optimization
    
  private:
    RendererType renderer_type;
    
  };
  
}; //mip