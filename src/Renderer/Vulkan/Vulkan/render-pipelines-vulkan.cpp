#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <Vulkan/render-pipelines-vulkan.h>

using namespace ilen::render;

VulkanRenderPipeline::VulkanRenderPipeline(vk::Device device, const IlenRenderPipelineDescriptor& descriptor, resources::shaders::ShaderManager* shaderManager){

}

VulkanRenderPipeline::~VulkanRenderPipeline(){

}

const void* VulkanRenderPipeline::getNativeRenderPipeline() const {
    return *_pso;
}

size_t VulkanRenderPipeline::getFormatSize(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format){

}

vk::Format VulkanRenderPipeline::getVertexFormat(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format){

}

#endif
