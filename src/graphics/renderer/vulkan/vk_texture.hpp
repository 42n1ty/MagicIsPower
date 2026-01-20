#pragma once

#include "../../i_texture.hpp"
#include "vk_image.hpp"


namespace mip {
  
  /**
   * @brief ITexture implementation for Vulkan
   * Manages the vk::Image, vk::ImageView, and vk::Sampler resources
   */
  class VulkanTexture : public ITexture {
  public:
    
    VulkanTexture();
    ~VulkanTexture();
    
    bool init(
      const std::string& path,
      vk::raii::PhysicalDevice& pDev,
      vk::raii::Device& lDev,
      vk::raii::CommandPool& cmdPool,
      vk::raii::Queue& graphQ
    );
    
    vk::raii::ImageView& getImgView() { return m_texImgView; }
    vk::raii::Sampler& getSampler() { return m_texSampler; }
    
    std::string s_path;
  
  private:
    
    bool createTextureImg(
      const std::string& path,
      vk::raii::PhysicalDevice& pDev,
      vk::raii::Device& lDev,
      vk::raii::CommandPool& cmdPool,
      vk::raii::Queue& graphQ
    );
    
    bool createTextureImgView(vk::raii::Device& lDev);
    
    bool createTextureSampler(
      vk::raii::PhysicalDevice& pDev,
      vk::raii::Device& lDev
    );
    
    vk::raii::Image m_texImg{nullptr};
    vk::raii::DeviceMemory m_texImgMem{nullptr};
    vk::raii::ImageView m_texImgView{nullptr};
    vk::raii::Sampler m_texSampler{nullptr};
    
  };
  
  
}; //mip