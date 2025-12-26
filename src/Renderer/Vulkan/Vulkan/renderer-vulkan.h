#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/render/renderer.h>
#include <vulkan/vulkan_raii.hpp>
#include <ilen/window/window.h>

#include <Vulkan/shaders-vulkan.h>
#include <Vulkan/texture-vulkan.h>

#if ILEN_LINUX
#include <window/window-linux.h>
#endif

namespace ilen::render {
    class VulkanRender final: public IRenderer {
        public:
            VulkanRender(const void* native_surface);
            ~VulkanRender() override;
            void beginFrame() override;
            void endFrame() override;
            void endEncoding(IGPUParallelCommandEncoder* encoder) override;
            void regenerateEncoderDescriptor() override;

            //Builder for GPU Interfaces
            std::shared_ptr<IGPUBuffer> createBuffer(const IlenBufferDescriptor& descriptor, const void* data) override;
            std::shared_ptr<IShader> createShader(const ilen::graphics::IlenShader& shaderDescriptor) override;
            std::shared_ptr<IGPURenderPipeline> createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor& pipelineDescriptor, ilen::resources::shaders::ShaderManager* shaderManager) override;
            std::unique_ptr<IGPUParallelCommandEncoder> createNewParallelCommandEncoder(IlenParallelEncoderDescriptor&& descriptor) override;
            std::unique_ptr<IGPUTexture> createNewPlatformTexture(const IlenGPUTextureDescriptor& descriptor) override;
            std::unique_ptr<IGPUInstanceBuffer>createInstanceBuffer(size_t bufferSize, std::string bufferName) override;

            //getters of encoders and buffers.
            std::unique_ptr<IGPUParallelCommandEncoder> getParallelCommandEncoder() override;
            void returnParallelCommandEncoder(std::unique_ptr<IGPUParallelCommandEncoder> encoder) override;
            IGPUInstanceBuffer* getInstanceBuffer() override;
            IGPUTexture* getDrawableTexturePtr() override;
            std::vector<IGPUTexture*> getAllSceneTexturesPtr() override;


        private:
            uint8_t MAX_FRAMES_IN_FLIGHT = 3;
            uint32_t vulkanTargetVersion = vk::ApiVersion13; //Apuntar inicialmente a vulkan 1.3
            uint32_t graphicsQueueFamily = UINT32_MAX;
            uint32_t presentQueueFamily = UINT32_MAX;
            vk::Format swapchainImageFormat = vk::Format::eUndefined;
            vk::Extent2D swapchainExtent;

            bool enableValidationLayers = false;

            //Todo lo que requiere vulkan para funcionar (Un buen de cosas, AYUDA!!)
            vk::raii::SurfaceKHR surface = nullptr; //Superficie de Vulkan.
            vk::raii::Context context; //Contexto de Vulkan
            vk::raii::Instance instance = nullptr; //Instancia de la app de Vulkan
            vk::raii::PhysicalDevice physicalDevice = nullptr; //Dispositivo Físico (GPU) a usar.
            vk::raii::Device logicalDevice = nullptr; //Abstracción del dispositivo para ser usado por el motor.
            vk::raii::Queue graphicsQueue = nullptr; //Queue que se encargara de encolar el trabajo de gráficos.
            vk::raii::Queue presentQueue = nullptr; //Queue de presentación (normalmente es el mismo de gráficos).
            vk::raii::SwapchainKHR swapchain = VK_NULL_HANDLE; //Swapchain de la app
            std::vector<vk::Image> swapchainImages; //Esta son las únicas Imagenes (texturas) que mantenemos en el formato nativo de Vulkan en vez de en el propio del motor, al ser las que se presentan en pantalla.
            std::vector<std::unique_ptr<IGPUTexture>> swapchainTextures; //Version del motor de las texturas del swapchain de Vulkan, para uso interno del motor. Estas devuelven un VkImageView en su funcion de retorno del handler.
#if ILEN_DEBUG
            vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger = nullptr;
            static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageType,
            const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*);

            void setVulkanDebugCalls();
#endif

            //Private functions of the implementation, to make more specific functions
            void createVulkanInstance();
            void createVulkanSurface(const void* native_surface);
            void selectVulkanPhysicalDevice();
            void createVulkanLogicalDevice();
            void setVulkanQueueFamilies();
            void createSwapchain(); //Función privada que configura las opciones de la superficie para crear el swapchain.
            void createVulkanImageViews();
    };
}
#endif
