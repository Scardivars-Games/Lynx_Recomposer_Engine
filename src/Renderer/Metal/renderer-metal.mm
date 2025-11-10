#if defined (__APPLE__)
//Este código es uniforme para todas las plataformas de Apple (macOS y iOS de momento)
#include "renderer-metal.h"
#include "buffers-metal.h"
#include "shaders-metal.h"
#include "render-pipelines-metal.h"
#include "parallel-command-encoder-metal.h"
#include "instance-buffer-metal.h"
#include <ilen/headers/ilen-logs.h>

#include <TargetConditionals.h>

#import <MetalKit/MetalKit.h>
#import <Metal/Metal.h>

namespace ilen::render {

    //TODO: ADD Metal 4 support to use the more efficients encoders!!!
    MetalRender::MetalRender(void* native_surface){
        @autoreleasepool {
            metalView = (__bridge MTKView*)native_surface;
            _device = MTLCreateSystemDefaultDevice();
            if(!_device){
                //Es imposible crear el dispositivo de Metal!!! (Fallback a OpenGL)
                ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se logro crear el contexto de Metal. Dispositivo inválido");
            }
            metalView.device = _device;
            _commandQueue = [_device newCommandQueue];

            #if TARGET_OS_OSX
            metalView.clearColor = MTLClearColor(0.1, 0.1, 0.2, 1.0);
            #elif TARGET_OS_IOS
            metalView.clearColor = MTLClearColor(0.5, 0.2, 0.2, 1.0);
            #endif //TARGET_OS_IOS
            
            #if ILEN_MACOS
            //Cargar .metallib
            NSString* libraryPath = [[NSBundle mainBundle] pathForResource:@"ilen" ofType:@"metallib" inDirectory:@"shader-code"];
            #elif ILEN_IOS
            NSString* libraryPath = [[NSBundle mainBundle] pathForResource:@"ilen" ofType:@"metallib" inDirectory:@"assets/shader-code"];
            #endif
            
            size_t instanceBufferSize = 4 * 1024 * 1024; //4 MB (con eso es suficiente para pruebas y así no me asusto tanto por el brindo de RAM. A futuro cambiar a 16)
            _instanceBuffer = std::make_unique<MetalInstanceBuffer>(_device, instanceBufferSize);
            
            if(!libraryPath){
                ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se encontro libreria por defecto de Metal");
                generateNewMetalLibrary();
            }else{
                NSURL* libraryURL = [NSURL fileURLWithPath:libraryPath];
                NSError *localError = nil;
                _ilenMetalLibrary = [_device newLibraryWithURL:libraryURL error:&localError];
                if(!_ilenMetalLibrary){
                    ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Imposible cargar libreria por defecto del motor %@", localError);
                }
            }
        }
    }

    MetalRender::~MetalRender() {
        if(_parallelCommandEncoder){
            IRenderer::FinalizeEncoding(_parallelCommandEncoder.get());
            _parallelCommandEncoder.reset();
        }
        _device = nil;
        _commandQueue = nil;
        _ilenMetalLibrary = nil;
    }

    void MetalRender::beginFrame() {
        _frameAutoReleasePool = [[NSAutoreleasePool alloc] init];
        _instanceBuffer->OnBeginFrame();
        
        _commandBuffer = [_commandQueue commandBuffer];
        _parallelCommandEncoder = std::make_unique<MetalParallelCommandEncoder>(_commandBuffer, metalView.currentRenderPassDescriptor);
    }

    void MetalRender::endFrame() {
            //No necesario en METAL
            if(_parallelCommandEncoder){
                IRenderer::FinalizeEncoding(_parallelCommandEncoder.get());
                _parallelCommandEncoder.reset();
            }else{
                ILEN_ERROR_P(DEBUG_PREFIX, "Llamado el end frame sin un válido parallel encoder creado");
            }
                
            [_commandBuffer presentDrawable:metalView.currentDrawable];
            [_commandBuffer commit];
            
            _commandBuffer = nil;
        
            [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
            _frameAutoReleasePool = nullptr;
    }
        
    std::shared_ptr<IGPUBuffer> MetalRender::createBuffer(const IlenBufferDescriptor& descriptor, const void* data) {
        //Vamos a crear primero un stagging buffer y luego crear el buffer real que manejara la app
        return std::make_shared<MetalBuffer>(_device, descriptor, data, _commandQueue);
    }

    std::shared_ptr<IShader> MetalRender::createShader(const ilen::graphics::IlenShader& shaderDescriptor){
        return std::make_shared<MetalShader>(_ilenMetalLibrary, shaderDescriptor);
    }

    std::shared_ptr<IGPURenderPipeline> MetalRender::createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor &pipelineDescriptor, ilen::resources::shaders::ShaderManager *shaderManager){
        return std::make_shared<MetalRenderPipeline>(_device, pipelineDescriptor, shaderManager, metalView);
    }

    IGPUParallelCommandEncoder* MetalRender::getParallelCommandEncoder(){
        return _parallelCommandEncoder.get();
    }

    IGPUInstanceBuffer* MetalRender::getInstanceBuffer() {
        return _instanceBuffer.get();
    }


    void MetalRender::generateNewMetalLibrary() {
        //Método fallback que busca todos los archivos fuente de .metal y los compila como un único .metallib
    }

    /*
    Renderer* Renderer::create(void* native_surface){
        return new MetalRender(native_surface);
    }*/
}

#endif // defined (__APPLE__)

