#include "vk_material.hpp"
#include "vk_swapchain.hpp"

namespace mip {
  
  VulkanMaterial::VulkanMaterial() {}
  VulkanMaterial::~VulkanMaterial() {}
    
  bool VulkanMaterial::init(
    const VulkanPplConfig& config,
    vk::raii::Device& lDev,
    VulkanSwapchain& sc,
    vk::raii::DescriptorSetLayout& perFrameLayout, // layout set=0
    vk::raii::DescriptorSetLayout& perMaterialLayout, // layout set=1
    vk::raii::DescriptorPool& descPool,
    vk::Format depthFormat
  ) {
    m_lDev = &lDev;
    
    std::array<vk::DescriptorSetLayout, 2> setLayouts = {perFrameLayout, perMaterialLayout};
    
    vk::PushConstantRange pcRange{
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
      .offset = 0,
      .size = sizeof(glm::mat4)
    };
    
    vk::PipelineLayoutCreateInfo plInfo{
      .setLayoutCount = setLayouts.size(),
      .pSetLayouts = setLayouts.data(),
      
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pcRange
    };
    
    if(!m_pipeline.init(lDev, sc, plInfo, depthFormat, config)) {
      Logger::error("Failed to create pipeline for material");
      return false;
    }
    
    vk::DescriptorSetAllocateInfo allocInfo{
      .descriptorPool = descPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &(*perMaterialLayout)
    };
    
    auto res = lDev.allocateDescriptorSets(allocInfo);
    if(!res.has_value()) {
      Logger::error("Failed to allocate material descriptor set: {}", vk::to_string(res.result));
      return false;
    }
    m_descSet = std::move(res.value[0]);
    
    return true;
  }
  
  void VulkanMaterial::setTexture(int slot, std::shared_ptr<ITexture> texture) {
    m_texture = static_pointer_cast<VulkanTexture>(texture);
    
    vk::DescriptorImageInfo imgInfo{
      .sampler = m_texture->getSampler(),
      .imageView = m_texture->getImgView(),
      .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    
    vk::WriteDescriptorSet descWrite{
      .dstSet = *m_descSet,
      .dstBinding = 0, // binding 0 in set=1
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = &imgInfo
    };
    
    m_lDev->updateDescriptorSets({descWrite}, {});
    
  }
  
  void VulkanMaterial::bind(vk::raii::CommandBuffer& cmdBuf) {
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.getPipeline());
    
    cmdBuf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      m_pipeline.getPipLayout(),
      1, // set=1
      {*m_descSet},
      {}
    );
  }
  
}; //mip