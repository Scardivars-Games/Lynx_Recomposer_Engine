#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/headers/enumerations.h>
#include <ilen/render/gpu_interfaces/render_pipelines.h>
#include <ilen/resources/shaders_manager.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <mutex>

namespace ilen::render {
    using namespace ilen::enumerations;

    struct VulkanVertexInputResources {
        std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
        vk::PipelineVertexInputStateCreateInfo createInfo;

        void sync() {
            createInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
            createInfo.pVertexBindingDescriptions = bindingDescriptions.data();
            createInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
            createInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        }
    };

    struct VulkanBindingKey {
        uint32_t binding;
        vk::DescriptorType descriptorType;
        uint32_t count;
        vk::ShaderStageFlags stages;

        auto operator<=>(const VulkanBindingKey&) const = default;
    };

    struct VulkanDescriptorSetKey {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        auto operator<=>(const VulkanDescriptorSetKey &) const = default;
    };

    struct VulkanPipelineLayoutkey {
        std::vector<vk::DescriptorSetLayout> setLayouts;
        std::vector<vk::PushConstantRange> pushConstants;
        auto operator<=>(const VulkanPipelineLayoutkey &) const = default;
    };

    class VulkanRenderPipeline final: public IGPURenderPipeline {
    public:
        explicit VulkanRenderPipeline(vk::raii::Device& device, const IlenRenderPipelineDescriptor& descriptor, resources::shaders::ShaderManager* shaderManager, vk::Extent2D swapchainExtent, IlenParallelEncoderDescriptor inUseGeneralEncoder, vk::raii::RenderPass& renderPass);
        //~VulkanRenderPipeline() override;
        const void* getNativeRenderPipeline() const override;

        static size_t getFormatSize(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format);
        static vk::Format getVertexFormat(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format);
        vk::DescriptorType getDescriptorType(structs::ILEN_VULKAN_DESCRIPTOR_TYPE type);
    private:
        vk::PipelineLayout getCachedPipelineLayout();

        vk::Device* device = nullptr;
        vk::raii::Pipeline _pso = nullptr;
        static std::map<VertexBufferLayout, std::unique_ptr<VulkanVertexInputResources>> _vertexBufferDescriptors;
        static std::map<VulkanBindingKey, vk::DescriptorSetLayoutBinding> _layoutBindings;
        static std::map<VulkanDescriptorSetKey, vk::DescriptorSetLayout> _descriptorSetLayouts;
        static std::map<VulkanPipelineLayoutkey, vk::PipelineLayout> _pipelineLayouts;
        static std::mutex _psoMutex;

        IlenRenderPipelineDescriptor _psoDescriptor;

        static inline auto DEBUG_PREFIX = "VULKAN_RENDER_PIPELINE";
        friend class IGPURenderPipeline;
    };
}

#endif
