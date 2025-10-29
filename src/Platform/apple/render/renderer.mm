#if defined(__APPLE__)

#include <ilen/render/renderer.h>
//Available renderers (METAL)
#include <renderer-metal.h>

namespace ilen::render {

    std::unique_ptr<IRenderer> IRenderer::CreateRenderer(void *native_surface, GraphicsAPI api){
        api = GraphicsAPI::Metal; //Solo soporta metal, así que da igual esto. Sería solamente determinar si puede usar Metal4 o Metal antiguo.
        
        return std::make_unique<MetalRender>(native_surface);
    }
}
#endif
