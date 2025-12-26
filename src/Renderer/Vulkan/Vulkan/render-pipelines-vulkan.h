#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/headers/enumerations.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>
#include <ilen/resources/shaders-manager.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <mutex>

namespace ilen::render {
    using namespace ilen::enumerations;

    class VulkanRenderPipeline final: public IGPURenderPipeline {
    public:
        explicit VulkanRenderPipeline(vk::Device device, const IlenRenderPipelineDescriptor& descriptor, resources::shaders::ShaderManager* shaderManager);
        ~VulkanRenderPipeline() override;
        const void* getNativeRenderPipeline() const override;

        static size_t getFormatSize(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format);
        static vk::Format getVertexFormat(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format);
    private:
        vk::raii::PipelineLayout _pso = nullptr;
        static std::map<VertexBufferLayout, void*> _vertexBufferDescriptors;
        static std::mutex _psoMutex;

        static inline auto DEBUG_PREFIX = "VULKAN_RENDER_PIPELINE";
        friend class IGPURenderPipeline;
    };
}

#endif
