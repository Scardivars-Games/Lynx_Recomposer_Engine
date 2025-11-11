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
            metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
            metalView.preferredFramesPerSecond = 60;
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
            
            _inFlightSemaphores = dispatch_semaphore_create(MAX_FRAMES_IN_FLIGHT);
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
        dispatch_semaphore_wait(_inFlightSemaphores, DISPATCH_TIME_FOREVER);
        _frameAutoReleasePool = [[NSAutoreleasePool alloc] init];
        _instanceBuffer->OnBeginFrame();
        
        _currentDrawable = [metalView currentDrawable];
        
        if(!_currentDrawable){
            _commandBuffer = nil;
            [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
            _frameAutoReleasePool = nil;
            return;
        }
        
        //Crear un Render Pass por defecto para opacos y depth buffer
        MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        //Color buffer
        renderPassDescriptor.colorAttachments[0].texture = _currentDrawable.texture;
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPassDescriptor.colorAttachments[0].clearColor = metalView.clearColor;
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        
        //Depth buffer
        renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
        renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
        renderPassDescriptor.depthAttachment.clearDepth = 1.0;
        renderPassDescriptor.depthAttachment.texture = metalView.depthStencilTexture;
        
        _commandBuffer = [_commandQueue commandBuffer];
        _parallelCommandEncoder = std::make_unique<MetalParallelCommandEncoder>(_commandBuffer, renderPassDescriptor);
    }

    void MetalRender::endFrame() {
        if(!_commandBuffer){
            if(_currentDrawable == nil){
                dispatch_semaphore_signal(_inFlightSemaphores);
            }
            return;
        }
        
            //No necesario en METAL
            if(_parallelCommandEncoder){
                IRenderer::FinalizeEncoding(_parallelCommandEncoder.get());
                _parallelCommandEncoder.reset();
            }else{
                ILEN_ERROR_P(DEBUG_PREFIX, "Llamado el end frame sin un válido parallel encoder creado");
            }
            
            __block dispatch_semaphore_t semaphore = _inFlightSemaphores;
        
            [_commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
                dispatch_semaphore_signal(semaphore);
            }];
                
            [_commandBuffer presentDrawable:_currentDrawable];
            [_commandBuffer commit];
            
            _commandBuffer = nil;
            _currentDrawable = nil;
        
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

