#if defined (__APPLE__)
//Este código es uniforme para todas las plataformas de Apple (macOS y iOS de momento)
#include <ilen/render/renderer.h>
#include <ilen/core/logs.h>

#include <TargetConditionals.h>

#import <MetalKit/MetalKit.h>
#import <Metal/Metal.h>

namespace ilen::render {
    class MetalRender : public Renderer {
        public:
            id<MTLDevice> device;
            id<MTLCommandQueue> commandQueue;
            MTKView* metalView;

            MetalRender(void* native_surface){
                metalView = (__bridge MTKView*)native_surface;
                device = MTLCreateSystemDefaultDevice();
                if(!device){
                    //Es imposible crear el dispositivo de Metal!!! (Fallback a OpenGL)
                    core::Debug::ExecuteError("No se logro crear el contexto de Metal. Dispositivo inválido", DEBUG_PREFIX);
                }
                metalView.device = device;
                commandQueue = [device newCommandQueue];

                #if TARGET_OS_OSX
                metalView.clearColor = MTLClearColor(0.1, 0.1, 0.2, 1.0);
                #elif TARGET_OS_IOS
                metalView.clearColor = MTLClearColor(0.5, 0.2, 0.2, 1.0);
                #endif //TARGET_OS_IOS
            }

            ~MetalRender() override {

            }

            void beginFrame() override {
                @autoreleasepool {
                    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
                    id<MTLRenderCommandEncoder> renderEnconder = [commandBuffer renderCommandEncoderWithDescriptor:metalView.currentRenderPassDescriptor];

                    [renderEnconder endEncoding];
                    [commandBuffer presentDrawable:metalView.currentDrawable];
                    [commandBuffer commit];
                }
            }

            void endFrame() override {
                //No necesario en METAL
            }
    };


    Renderer* Renderer::create(void* native_surface){
        return new MetalRender(native_surface);
    }
}

#endif //__APPLE__
