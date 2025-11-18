#if ILEN_LINUX
#include <ilen/render/renderer.h>
#include <renderer-vulkan.h>

using namespace ilen::render;

ILEN_GPU_GRAPHICS_API_TYPE IRenderer::actualAPI;

std::unique_ptr<IRenderer> IRenderer::CreateRenderer(void* native_surface, ILEN_GPU_GRAPHICS_API_TYPE api){
    //This is linux, only can use is Vulkan or OpenGL. Preferred Vulkan than Open GL.
    if(api == ILEN_GPU_GRAPHICS_API_TYPE::Auto){
        api = ILEN_GPU_GRAPHICS_API_TYPE::Vulkan;
        //Aquí trataria de ver si funciona
    }

    actualAPI = api;

    return std::make_unique<VulkanRender>(native_surface);
}
#endif