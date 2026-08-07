#pragma once

#include "../graphics/i_renderer.hpp"
#include <string>
#include <expected>
#include <memory>


namespace game {
  
  struct TextureLoader {
    mip::IRenderer* renderer;
    
    std::expected<std::shared_ptr<mip::ITexture>, std::string> load(const std::string& path) {
      auto tex = renderer->createTexture(path, false);
      if(tex) {
        return tex;
      }
      
      return std::unexpected("Failed to load texture" + path);
    }
  };
  
  struct MeshLoader {
    std::expected<std::shared_ptr<mip::IMesh>, std::string> load(const std::string&) {
      return std::unexpected("Meshes use insertAsset, not loadAsset");
    }
  };
  
  struct MaterialLoader {
    std::expected<std::shared_ptr<mip::IMaterial>, std::string> load(const std::string&) {
      return std::unexpected("Materials use insertAsset, not loadAsset");
    }
  };
  
}; //game