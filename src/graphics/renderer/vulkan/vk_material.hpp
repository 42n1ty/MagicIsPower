#pragma once

#include "../../i_material.hpp"
#include "vk_pipeline.hpp"
#include "vk_texture.hpp"



namespace mip {
  
  class VulkanSwapchain;
  
  /**
   * @brief IMaterial implementation for Vulkan
   * Links the pipeline (shaders) to textures via descriptor sets
   */
  class VulkanMaterial : public IMaterial {
  public:
    
    VulkanMaterial();
    ~VulkanMaterial();
    
    bool init(
      const VulkanPplConfig& config,
      vk::raii::Device& lDev,
      VulkanSwapchain& sc,
      vk::raii::DescriptorSetLayout& perFrameLayout, // layout set=0
      vk::raii::DescriptorSetLayout& perMaterialLayout, // layout set=1
      vk::raii::DescriptorPool& descPool,
      vk::Format depthFormat
    );
    
    /**
     * @brief Interface setTexture implementation
     */
    void setTexture(int slot, std::shared_ptr<ITexture> texture) override;
    void bind(vk::raii::CommandBuffer& cmdBuf);
    
    vk::raii::PipelineLayout& getPipLayout() { return m_pipeline.getPipLayout(); }
    
  private:
    
    std::shared_ptr<VulkanTexture> m_texture;
    VulkanPipeline m_pipeline;
    vk::raii::DescriptorSet m_descSet{nullptr};
    vk::raii::Device* m_lDev{nullptr};
    
  };
  
}; //mip