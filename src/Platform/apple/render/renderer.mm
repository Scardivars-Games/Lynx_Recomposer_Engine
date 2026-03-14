#if defined(__APPLE__)

#include <ilen/render/renderer.h>
//Available renderers (METAL)
#include <Metal/renderer-metal.h>

namespace ilen::render {
     ILEN_GPU_GRAPHICS_API_TYPE IRenderer::actualAPI;

    std::unique_ptr<IRenderer> IRenderer::CreateRenderer(const void *native_surface, ILEN_GPU_GRAPHICS_API_TYPE api){
        api = ILEN_GPU_GRAPHICS_API_TYPE::Metal; //Solo soporta metal, así que da igual esto. Sería solamente determinar si puede usar Metal4 o Metal antiguo.
        actualAPI = api;
        return std::make_unique<MetalRender>(native_surface);
    }
}
#endif
