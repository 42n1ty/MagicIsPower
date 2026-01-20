#pragma once
#include <memory>


namespace mip {
  
  class ITexture;
  
  /**
   * @brief Material Interface
   * Rules HOW to render object
   */
  class IMaterial {
  public:
    virtual ~IMaterial() = default;
    
    /**
     * @brief Binds a texture to a specific material slot
     * @param slot Texture slot (e.g. 0 for diffuse, 1 for specular etc.)
     * @param texture Texture pointer
     */
    virtual void setTexture(int slot, std::shared_ptr<ITexture> texture) = 0;
  };
  
}; //mip