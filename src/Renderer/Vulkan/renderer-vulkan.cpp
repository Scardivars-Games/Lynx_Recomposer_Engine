#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <renderer-vulkan.h>

using namespace ilen::render;

VulkanRender::VulkanRender(void *native_surface){
    //El objetivo de esto es generar hasta el dispositivo logico de la app, o sea, muchas cosas
    //Creemos los datos de la instancia
    vk::ApplicationInfo appInfo {
        .pApplicationName = application::Application::getApplicationName().c_str(),
        .applicationVersion = VK_MAKE_VERSION(application::Application::getMajorVersion(), application::Application::getMinorVersion(), application::Application::getPatchVersion()),
        .pEngineName = "Ilusion's Engine",
        .engineVersion =  VK_MAKE_VERSION(1,0,0),
        .apiVersion =  vk::ApiVersion10,
    };


}

VulkanRender::~VulkanRender() {

}

void VulkanRender::beginFrame(){

}

void VulkanRender::endFrame() {

}

std::shared_ptr<IGPUBuffer> VulkanRender::createBuffer(const IlenBufferDescriptor &descriptor, const void *data){

}

std::unique_ptr<IGPUTexture> VulkanRender::createNewPlatformTexture(const IlenGPUTextureDescriptor &descriptor){

}

std::shared_ptr<IGPURenderPipeline> VulkanRender::createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor &pipelineDescriptor, ilen::resources::shaders::ShaderManager *shaderManager){

}

std::shared_ptr<IShader> VulkanRender::createShader(const ilen::graphics::IlenShader &shaderDescriptor){

}

void VulkanRender::endEncoding(IGPUParallelCommandEncoder *encoder){

}

IGPUInstanceBuffer * VulkanRender::getInstanceBuffer(){

}

std::unique_ptr<IGPUParallelCommandEncoder> VulkanRender::getParallelCommandEncoder(){

}

void VulkanRender::returnParallelCommandEncoder(std::unique_ptr<IGPUParallelCommandEncoder> encoder){

}

std::unique_ptr<IGPUParallelCommandEncoder> VulkanRender::createNewParallelCommandEncoder(const IlenParallelEncoderDescriptor &descriptor){

}


#endif
