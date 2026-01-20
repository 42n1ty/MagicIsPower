#pragma once

#include "../../i_renderer.hpp"
#include "vk_swapchain.hpp"
#include "vk_ubo.hpp"

#include <expected>

struct GLFWwindow;

namespace mip {
  
  class Window;
  
  /**
   * @brief IRenderer implementation w/ Vulkan API
   */
  class VulkanRenderer : public IRenderer {
  public:
    VulkanRenderer();
    ~VulkanRenderer();
    
    bool s_isInit{false};
    int s_frameNum{0};
    bool s_stopRendering{false};
    
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    
    bool init(Window& wnd) override;
    void shutdown() override;
    
    virtual std::shared_ptr<IMesh> createMesh(const MeshData& data) override;
    virtual std::shared_ptr<ITexture> createTexture(const std::string& path, const bool flip) override;
    virtual std::shared_ptr<IMaterial> createMaterial(const std::string& vertShaderPath, const std::string& fragShaderPath = "") override;
    
    virtual bool beginFrame(const CameraInfo& camera) override;
    virtual bool submit(std::shared_ptr<IMesh> mesh, std::shared_ptr<IMaterial> material, const glm::mat4& transform) override;
    // virtual bool submitAnimated(std::shared_ptr<IMesh> mesh, std::shared_ptr<IMaterial> material, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms) override;
    virtual bool endFrame() override;
    
    static void framebufferResizeCallback(GLFWwindow* wnd, int w, int h);
    
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void*
    ) {
      if(severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        Logger::error("Validation layer: type {}; msg: {}", vk::to_string(type), pCallbackData->pMessage);
      }
      
      return vk::False;
    }
    
    bool framebufferResized = false;
    
    // std::shared_ptr<IMesh> getGlobalQuad() {return m_globQuad;} //for 2D optimization
    
  private:
    
    // INIT FUNCS====================================================================================================
    bool createInstance();
    bool setupDM();
    bool pickPhysDev();
    bool createLogDev();
    bool createSurf(Window& wnd);
    bool createSwapchain(Window& wnd);
    bool createImgViews();
    bool createDescSetLayouts();
    bool createCmdPool();
    
    bool createUBO();
    
    // bool createDepthRes();
    
    bool createDescPool();
    bool createDescSets();
    bool createCmdBufs();
    bool createSyncObjs();
    // INIT FUNCS====================================================================================================
    
    // HELPERS FUNCS====================================================================================================
    std::vector<const char*> getReqExtensions();
    void printDev();
    // void recordCmdBuf(uint32_t index);
    void recreateSC();
    void cleanupSC();
    // HELPERS FUNCS====================================================================================================
    
    const std::vector<const char*> m_validLayers = {
      "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> m_devExtensions = {
      vk::KHRSwapchainExtensionName,
      vk::KHRSpirv14ExtensionName,
      vk::KHRSynchronization2ExtensionName,
      vk::KHRCreateRenderpass2ExtensionName
    };
    
    vk::raii::Context m_ctx;
    vk::raii::Instance m_inst{nullptr};
    vk::raii::DebugUtilsMessengerEXT m_debMesser{nullptr};
    vk::raii::PhysicalDevice m_physDev{nullptr};
    vk::raii::Device m_logDev{nullptr};
    vk::raii::Queue m_graphQ{nullptr};
    vk::raii::Queue m_presQ{nullptr};
    vk::raii::SurfaceKHR m_surf{nullptr};
    vk::raii::CommandPool m_cmdPool{nullptr};
    vk::raii::DescriptorSetLayout m_perFrameDescSetLayout{nullptr};
    vk::raii::DescriptorSetLayout m_perMatDescSetLayout{nullptr};
    vk::raii::DescriptorPool m_descPool{nullptr};
    
    // vk::raii::Image m_depthImg{nullptr};
    // vk::raii::DeviceMemory m_depthImgMem{nullptr};
    // vk::raii::ImageView m_depthImgView{nullptr};
    
    std::vector<vk::raii::CommandBuffer> m_cmdBufs;
    std::vector<vk::raii::Semaphore> m_presCompleteSems;
    std::vector<vk::raii::Semaphore> m_renderFinishedSems;
    std::vector<vk::raii::Fence> m_inFlightFences;
    std::vector<vk::Fence> m_imagesInFlight;
    std::vector<vk::raii::DescriptorSet> m_perFrameDescSets;
    
    vk::raii::CommandBuffer* m_curCmdBuf{nullptr};
    uint32_t m_curImgIndex{0};
    
    UBOManager<CameraData> m_cameraUBO;
    UBOManager<ObjectData> m_objectUBO;
    // UBOManager<BoneData> m_bonesUBO;
    
    VulkanSwapchain m_sc;
    
    std::vector<vk::raii::ImageView> m_imgViews;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastFrameTime;
    
    Window* m_wnd;
    
    // std::shared_ptr<IMesh> m_globQuad{nullptr}; // for 2D optimization
    
    size_t m_curFrame = 0;
    uint32_t m_graphQI;
    uint32_t m_presQI;
    
    #ifdef NDEBUG
      const bool enableValidLayers = false;
    #else
      const bool enableVaildLayers = true;
    #endif
  };
  
}; //mip