#pragma once

#include "../../i_mesh.hpp"
#include "vk_buffer.hpp"
#include "../../../common/vertex.hpp"

namespace mip {
  /**
   * @brief IMesh implementation for Vulkan
   * Stores and manages Vulkan vertex and index buffers
   */
  class VulkanMesh : public IMesh{
  public:
    
    VulkanMesh() {}
    ~VulkanMesh() = default;
    
    /**
     * @brief Mesh initialization
     * @param data Raw vertex/index data
     * @param pDev Vulkan physical device
     * @param lDev Vulkan device
     * @param cmdPool Vulkan command pool for copying operations
     * @param graphQ Vulkan queue for copying operations
     */
    bool init(
      const MeshData& data,
      vk::raii::PhysicalDevice& pDev,
      vk::raii::Device& lDev,
      vk::raii::CommandPool& cmdPool,
      vk::raii::Queue& graphQ
    ) {
      if(!createVBuf(
          data.vertices,
          pDev,
          lDev,
          cmdPool,
          graphQ
        )
        ||
        !createIBuf(
          data.indices,
          pDev,
          lDev,
          cmdPool,
          graphQ
        )
      ) return false;
      
      m_indCnt = data.indices.size();
      return true;
    }
    
    /**
     * @brief Binds vertex and index buffers to the command buffer
     * @param cmdBuf Active command buffer
     */
    void bind(vk::raii::CommandBuffer& cmdBuf) {
      cmdBuf.bindVertexBuffers(0, *m_vertBuf, {0});
      cmdBuf.bindIndexBuffer(*m_indBuf, 0, vk::IndexType::eUint32);
    }
    
    uint32_t getIndexCount() { return m_indCnt; }
    
  private:
    
    bool createVBuf(
      const std::vector<Vertex2D>& verts,
      vk::raii::PhysicalDevice& pDev,
      vk::raii::Device& lDev,
      vk::raii::CommandPool& cmdPool,
      vk::raii::Queue& graphQ
    ) {
      
      vk::DeviceSize bufSize = sizeof(verts[0]) * verts.size();
      vk::raii::Buffer stagingBuf{nullptr};
      vk::raii::DeviceMemory stagingBufMem{nullptr};
      if(!createBuf(
        bufSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuf,
        stagingBufMem,
        pDev,
        lDev
      )) return false;
      
      void* dataStaging = nullptr;
      {
        auto res = stagingBufMem.mapMemory(0, bufSize);
        if(!res.has_value()) {
          Logger::error("Failed to map buffer memory: {}", vk::to_string(res.result));
          return false;
        }
        dataStaging = std::move(*res);
      }
      memcpy(dataStaging, verts.data(), bufSize);
      stagingBufMem.unmapMemory();
      
      if(!createBuf(
        bufSize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_vertBuf,
        m_vertBufMem,
        pDev,
        lDev
      )) return false;
      
      copyBuffer(stagingBuf, m_vertBuf, bufSize, lDev, cmdPool, graphQ);
      
      return true;
    }
  
    bool createIBuf(
      const std::vector<uint32_t>& inds,
      vk::raii::PhysicalDevice& pDev,
      vk::raii::Device& lDev,
      vk::raii::CommandPool& cmdPool,
      vk::raii::Queue& graphQ
    ) {
      vk::DeviceSize bufSize = sizeof(inds[0]) * inds.size();
      vk::raii::Buffer stagingBuf{nullptr};
      vk::raii::DeviceMemory stagingBufMem{nullptr};
      if(!createBuf(
        bufSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuf,
        stagingBufMem,
        pDev,
        lDev
      )) return false;
      
      void* data = nullptr;
      {
        auto res = stagingBufMem.mapMemory(0, bufSize);
        if(!res.has_value()) {
          Logger::error("Failed to map buffer memory: {}", vk::to_string(res.result));
          return false;
        }
        data = std::move(*res);
      }
      memcpy(data, inds.data(), bufSize);
      stagingBufMem.unmapMemory();
      
      if(!createBuf(
        bufSize,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_indBuf,
        m_indBufMem,
        pDev,
        lDev
      )) return false;
      
      copyBuffer(stagingBuf, m_indBuf, bufSize, lDev, cmdPool, graphQ);
      
      return true;
    }
  
    
    vk::raii::Buffer m_vertBuf{nullptr};
    vk::raii::DeviceMemory m_vertBufMem{nullptr};
    vk::raii::Buffer m_indBuf{nullptr};
    vk::raii::DeviceMemory m_indBufMem{nullptr};
    uint32_t m_indCnt{0};
    
  };
  
}; //mip