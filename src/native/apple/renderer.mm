#if defined (__APPLE__)
//Este código es uniforme para todas las plataformas de Apple (macOS y iOS de momento)
#include <ilen/render/renderer.h>
#include <ilen/headers/ilen-logs.h>
#include <memory>

#include <TargetConditionals.h>

#import <MetalKit/MetalKit.h>
#import <Metal/Metal.h>

namespace ilen::render {
    class MetalBuffer : public IGPUBuffer {
        public:
            MetalBuffer(id<MTLBuffer> buffer, const IlenBufferDescriptor& descriptor) : _buffer(buffer), _descriptor(descriptor) {};
        
            ~MetalBuffer() override {
                _buffer = nil;
            }
            
            const IlenBufferDescriptor& getDescriptor() const override {
                return _descriptor;
            }
        
            void* getNativeHandle() const override {
                return (__bridge void*)_buffer;
            }
        private:
            id<MTLBuffer> _buffer;
            IlenBufferDescriptor _descriptor;
    };

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
                    ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se logro crear el contexto de Metal. Dispositivo inválido");
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
        
            std::shared_ptr<IGPUBuffer> createBuffer(const IlenBufferDescriptor& descriptor, const void* data = nullptr) override {
                MTLResourceOptions options = descriptor.cpuVisible ? MTLResourceStorageModeShared : MTLResourceStorageModePrivate;
                
                id<MTLBuffer> buffer;
                
                if(data == nullptr){
                    buffer = [device newBufferWithLength:descriptor.bufferSize options:options];
                }else{
                     buffer = [device newBufferWithBytes:data length:descriptor.bufferSize options: options];
                }
                
                return std::make_shared<MetalBuffer>(buffer, descriptor);
            }
    };

    Renderer* Renderer::create(void* native_surface){
        return new MetalRender(native_surface);
    }
}

#endif // defined (__APPLE__)
