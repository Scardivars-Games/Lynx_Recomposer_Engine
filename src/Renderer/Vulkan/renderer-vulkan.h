#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/render/renderer.h>
#include <vulkan/vulkan_raii.hpp>

namespace ilen::render {
    class VulkanRender final: public IRenderer {
        public:
            VulkanRender(void* native_surface);
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

            bool enableValidationLayers = false;

        //Todo lo que requiere vulkan para funcionar (Un buen de cosas, AYUDA!!)
        vk::raii::Context context; //Contexto de Vulkan
        vk::raii::Instance instance = nullptr; //Instancia de la app de Vulkan
        vk::raii::PhysicalDevice physicalDevice = nullptr; //Dispositivo Físico (GPU) a usar.
        vk::raii::Device logicalDevice = nullptr; //Abstracción del dispositivo para ser usado por el motor.
#if ILEN_DEBUG
      vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger = nullptr;
        static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*);
#endif
    };
}
#endif
