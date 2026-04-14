#if defined (__APPLE__)
//Este código es uniforme para todas las plataformas de Apple (macOS y iOS de momento)
#include <Metal/renderer-metal.h>
#include <Metal/buffers-metal.h>
#include <Metal/shaders-metal.h>
#include <Metal/render-pipelines-metal.h>
#include <Metal/parallel-command-encoder-metal.h>
#include <Metal/texture-metal.h>
#include <Metal/instance-buffer-metal.h>
#include <ilen/headers/ilen-logs.h>
#include <ilen/window/window.h>

#include <TargetConditionals.h>

#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>

namespace ilen::render {
    using namespace ilen::core::options;

    //TODO: ADD Metal 4 support to use the more efficients encoders!!!
    MetalRender::MetalRender(const void* native_surface){
        @autoreleasepool {
            //lastUsedGraphicsOptions = defaultOptions;
            
#ifdef ILEN_MACOS
            _metalView = (__bridge NSView*)native_surface;
#elif defined ILEN_IOS
            _metalView = (__bridge UIView*)native_surface;
#endif
            _metalLayer = (CAMetalLayer*)_metalView.layer;
            _device = _metalLayer.device;
            if(!_device){
                //Es imposible crear el dispositivo de Metal!!! (Fallback a OpenGL)
                ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se logro crear el contexto de Metal. Dispositivo inválido");
            }
            /*Irrelevante ahora que usamos CAMetalLayer
             metalView.device = _device;
            metalView.framebufferOnly = NO;
            metalView.drawableSize = metalView.bounds.size;
            metalView.paused = YES;
            metalView.enableSetNeedsDisplay = YES;
            metalView.autoResizeDrawable = YES;
            #if TARGET_OS_OSX
            metalView.needsDisplay = YES;
            [metalView setNeedsLayout:YES];
            [metalView layoutSubtreeIfNeeded];
             
             metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
             metalView.preferredFramesPerSecond = GraphicsOptions::actualFPS;
            #endif*/
            
            ILEN_LOG("Device: %p", _metalLayer.device);
            ILEN_LOG("DrawableSize: %.0f x %.0f", _metalLayer.drawableSize.width, _metalLayer.drawableSize.height);
            
           
            _commandQueue = [_device newCommandQueue];
            
            _predefinedValues.screenPixelFormat = _metalLayer.pixelFormat;
            _predefinedValues.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
            #if TARGET_OS_OSX
            _predefinedValues.clearColor = MTLClearColor(0.1, 0.1, 0.2, 1.0);
            #elif TARGET_OS_IOS
            _predefinedValues.clearColor = MTLClearColor(0.5, 0.2, 0.2, 1.0);
            #endif //TARGET_OS_IOS
            
            #if ILEN_MACOS
            //Cargar .metallib
            NSString* libraryPath = [[NSBundle mainBundle] pathForResource:@"ilen" ofType:@"metallib" inDirectory:@"shader-code"];
            #elif ILEN_IOS
            NSString* libraryPath = [[NSBundle mainBundle] pathForResource:@"ilen" ofType:@"metallib" inDirectory:@"assets/shader-code"];
            #endif
            
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
            
            _instanceBuffer.resize(MAX_FRAMES_IN_FLIGHT);
            _depthTexture.resize(MAX_FRAMES_IN_FLIGHT);
            _commandBuffer.resize(MAX_FRAMES_IN_FLIGHT);
            _drawables.resize(MAX_FRAMES_IN_FLIGHT);
            _drawableTexture.resize(MAX_FRAMES_IN_FLIGHT);
            _parallelCommandEncoder.resize(MAX_FRAMES_IN_FLIGHT);

            
            size_t instanceBufferSize = 4 * 1024 * 1024; //4 MB (con eso es suficiente para pruebas y así no me asusto tanto por el brindo de RAM. A futuro cambiar a 16)
            for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
                _instanceBuffer[i] = createInstanceBuffer(instanceBufferSize, "Instance Main Ring Buffer -> Frame " + std::to_string(_actualFrame));
            }
            
            _inFlightSemaphores = dispatch_semaphore_create(MAX_FRAMES_IN_FLIGHT);
            
            
            //Es importante que los command encoder
            
            int totalOfCustomTextures = 0;
            if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS){
                totalOfCustomTextures += MAX_FRAMES_IN_FLIGHT; //Estas texturas son los render target de los Pass Descriptor. La idea es, por cada pass que no dibuje en pantalla, crea las texturas, y luego se usa un índice que viene en el Constructor que doy para saber en que bloque de texturas leer. 0 sería la textura de pantalla (i * 0 = 0), así que es buena idea, y a partir del 1, pues se obtiene el índice de la custom texture.
                if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_MOTION_BLUR){
                    totalOfCustomTextures += MAX_FRAMES_IN_FLIGHT; //Para almacenar el velocity buffer en al attachment 1.
                }
            }
            
            _sceneTexture.resize(totalOfCustomTextures);
            generateFrameTextures();
            regenerateEncoderDescriptor();
        }
    }

    MetalRender::~MetalRender() {
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            if(_parallelCommandEncoder[i]){
                IRenderer::FinalizeEncoding(_parallelCommandEncoder[i].get());
                _parallelCommandEncoder[i].reset();
            }
        }

        _device = nil;
        _commandQueue = nil;
        _ilenMetalLibrary = nil;
    }

void MetalRender::beginFrame() {
    
    //        ILEN_LOG("Waitin for frame %i. Start time %.8f", _actualFrame, ilen::time::Time::getTimeSinceCurrentFrameStart());
    dispatch_semaphore_wait(_inFlightSemaphores, DISPATCH_TIME_FOREVER);
    //ILEN_LOG("Getted frame %i. End time %.8f", _actualFrame, ilen::time::Time::getTimeSinceCurrentFrameStart());
    
    _frameAutoReleasePool = [[NSAutoreleasePool alloc] init];
    generateFrameTextures();
    _instanceBuffer[_actualFrame]->OnBeginFrame();
    
    //_currentDrawable = [metalView currentDrawable];
    
    /*if(!_currentDrawable){
     _commandBuffer = nil;
     [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
     _frameAutoReleasePool = nil;
     return;
     }*/
    
    //_drawables[_actualFrame] = [metalView currentDrawable];
    
    //CAMetalLayer* layer = (CAMetalLayer*)[metalView layer];
    if(_metalLayer.drawableSize.width < 1 || _metalLayer.drawableSize.height < 1){
        _commandBuffer[_actualFrame] = nil;
        _drawables[_actualFrame] = nil;
        [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
        _frameAutoReleasePool = nil;
        return;
    }
    
    _drawables[_actualFrame] = [_metalLayer nextDrawable];
    
    
    if(!_drawables[_actualFrame]){
        _commandBuffer[_actualFrame] = nil;
        [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
        _frameAutoReleasePool = nil;
        return;
    }
    _drawableTexture[_actualFrame] = std::make_unique<MetalTexture>(_drawables[_actualFrame].texture);
    
    if(lastUsedGraphicsOptions != GraphicsOptions::graphicsOptionsFlags){
        regenerateEncoderDescriptor();
    }
    
    IlenParallelEncoderDescriptor parallelEncoderDescriptor = actualMainDescriptor;
    
    if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS){
        parallelEncoderDescriptor.colorAttachments[slots::RenderPassAttachment_Color]->texture = _sceneTexture[_actualFrame].get();
        uint32_t actualIndexForNextTexture = MAX_FRAMES_IN_FLIGHT;

        if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_MOTION_BLUR){
            parallelEncoderDescriptor.colorAttachments[slots::RenderPassAttachment_VelocityBuffer]->texture = _sceneTexture[_actualFrame + actualIndexForNextTexture].get();
            actualIndexForNextTexture += MAX_FRAMES_IN_FLIGHT;
        }
    }else{
        parallelEncoderDescriptor.colorAttachments[slots::RenderPassAttachment_Color]->texture = _drawableTexture[_actualFrame].get();
    }
    
    if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_DEPTH){
        parallelEncoderDescriptor.depthAttachment->texture = _depthTexture[_actualFrame].get();
    }
    
            
    _commandBuffer[_actualFrame] = [_commandQueue commandBuffer];
    _parallelCommandEncoder[_actualFrame] = std::make_unique<MetalParallelCommandEncoder>(_commandBuffer[_actualFrame], std::move(parallelEncoderDescriptor));
    }

    void MetalRender::endFrame() {
        if(isParallelEncoderInOtherObject){
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "The parallel encoder was given to another process, but never returned a parallel encoder to the renderer");
            return;
        }
        
        if(!_commandBuffer[_actualFrame]){
            if(_drawables[_actualFrame] == nil){
                dispatch_semaphore_signal(_inFlightSemaphores);
                _actualFrame = (_actualFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }
            return;
        }
    
        if(_parallelCommandEncoder[_actualFrame]){
            IRenderer::FinalizeEncoding(_parallelCommandEncoder[_actualFrame].get());
            _parallelCommandEncoder[_actualFrame].reset();
        }else{
            ILEN_ERROR_P(DEBUG_PREFIX, "Llamado el end frame sin un válido parallel encoder creado");
        }
        
        __block dispatch_semaphore_t semaphore = _inFlightSemaphores;
    
        [_commandBuffer[_actualFrame] addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
            dispatch_semaphore_signal(semaphore);
        }];
            
        [_commandBuffer[_actualFrame] presentDrawable:_drawables[_actualFrame]];
        [_commandBuffer[_actualFrame] commit];
//        ILEN_LOG("Showing frame %i", _actualFrame);
        
        _commandBuffer[_actualFrame] = nil;
       // [_drawables[_actualFrame] release];
        _drawables[_actualFrame] = nil;
        //_currentDrawable = nil;
    
        [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
        _frameAutoReleasePool = nullptr;
    
        _actualFrame = (_actualFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void MetalRender::endEncoding(IGPUParallelCommandEncoder *encoder){
        if(encoder == nullptr){
            ILEN_ERROR_P(DEBUG_PREFIX, "Called to end the encoding to a null Parallel command encoder.");
        }
        
        IRenderer::FinalizeEncoding(encoder);
        
    }
        
    std::shared_ptr<IGPUBuffer> MetalRender::createBuffer(const IlenBufferDescriptor& descriptor, const void* data) {
        //Vamos a crear primero un stagging buffer y luego crear el buffer real que manejara la app
        return std::make_shared<MetalBuffer>(_device, descriptor, data, _commandQueue);
    }

    std::shared_ptr<IShader> MetalRender::createShader(const ilen::graphics::IlenShader& shaderDescriptor){
        return std::make_shared<MetalShader>(_ilenMetalLibrary, shaderDescriptor);
    }

    std::shared_ptr<IGPURenderPipeline> MetalRender::createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor &pipelineDescriptor, ilen::resources::shaders::ShaderManager *shaderManager){
        return std::make_shared<MetalRenderPipeline>(_device, pipelineDescriptor, shaderManager, actualMainDescriptor, _predefinedValues);
    }

    std::unique_ptr<IGPUParallelCommandEncoder> MetalRender::getParallelCommandEncoder(){
        isParallelEncoderInOtherObject = true;
        return std::move(_parallelCommandEncoder[_actualFrame]);
    }

    void MetalRender::returnParallelCommandEncoder(std::unique_ptr<IGPUParallelCommandEncoder> encoder){
        _parallelCommandEncoder[_actualFrame] = std::move(encoder);
        isParallelEncoderInOtherObject = false;
    }

    IGPUInstanceBuffer* MetalRender::getInstanceBuffer() {
        return _instanceBuffer[_actualFrame].get();
    }


    void MetalRender::generateNewMetalLibrary() {
        //Método fallback que busca todos los archivos fuente de .metal y los compila como un único .metallib
    }

//ADVERTENCIA: La lógica de esto hay que reescribirla. Porque, y si el usuario activa el postprocesado, añade 3 texturas más a las scene texture, pero ya el valor [0] tiene algo escrito, porque por algun proceso el motor requeria otro Render Pass. En ese caso, al usar el indice de los target que necesita el post procesado, no encotrara nada!!!.
void MetalRender::generateFrameTextures(){
    @autoreleasepool {
        auto width = ilen::window::IWindow::getWidth();
        auto height = ilen::window::IWindow::getHeight();
        
        bool haveTextures = _sceneTexture.size() > 0;
        bool haveDepth = _depthTexture.size() > 0;
        
        if(!(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS) && !(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_DEPTH)
           && !(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_STENCIL)){
            return;
        }
        
        if((haveTextures && !_sceneTexture[0]) || (haveTextures && _sceneTexture[0]->getTexWidth() != width && _sceneTexture[0]->getTexHeight() != height)){
            for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){ //Se calcula con respecto a los Frames in flight por seguridad. Si aquí almaceno referencia a todas las texturas que usa mi motor, usando como distancia lógica los frames in flight, por cada frame in flight, y reglas claras de como generar las texturas, debo ir creándolas
                
                int advacedSteps = 0; //Para calcular se deben ser las texturas 0 - 2, o 3 - 5, o 6 - 8
                if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS){
                    _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)].reset(); //Para forzar la liberación de cualquier textura de antes.
                    std::string label = "Pre-PostProcess Texture Data - Frame " + std::to_string(i);
                    //Generamos la textura para dibujar
                    IlenGPUTextureDescriptor descriptor{};
                    descriptor.height = height;
                    descriptor.width = width;
                    descriptor.presentation = enumerations::ILEN_GPU_TEXTURE_PRESENTATION_TYPE::ILEN_GPU_TEXTURE_PRESENTATION_NOTPRESENT;
                    descriptor.pixelFormat = enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_SCREEN;
                    descriptor.usage = ILEN_GPU_TEXTURE_USAGE_RENDERTARGET | ILEN_GPU_TEXTURE_USAGE_SHADER_READ;
                    descriptor.memoryProperty = enumerations::ILEN_GPU_MEMORY_PROPERTY_TYPE::ILEN_GPU_MEMORY_PROPERTY_PRIVATE;
                    descriptor.label = label;
                    
                    _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)] = createNewPlatformTexture(descriptor);
                    advacedSteps++;
                    
                    //Post process effect requerided images!!!
                    if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_MOTION_BLUR){
                        //Generate the motion texture
                        _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)].reset(); //Para forzar la liberación de cualquier textura de antes.

                        label = "Motion Texture for Post Process - Frame " + std::to_string(i);
                        IlenGPUTextureDescriptor descriptor{};
                        descriptor.height = height;
                        descriptor.width = width;
                        descriptor.presentation = enumerations::ILEN_GPU_TEXTURE_PRESENTATION_TYPE::ILEN_GPU_TEXTURE_PRESENTATION_NOTPRESENT;
                        descriptor.pixelFormat = enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2_HALF;
                        descriptor.usage = ILEN_GPU_TEXTURE_USAGE_RENDERTARGET | ILEN_GPU_TEXTURE_USAGE_SHADER_READ;
                        descriptor.memoryProperty = enumerations::ILEN_GPU_MEMORY_PROPERTY_TYPE::ILEN_GPU_MEMORY_PROPERTY_PRIVATE;
                        descriptor.label = label;
                        
                        _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)] = createNewPlatformTexture(descriptor);
                    }
                }else{
                    _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)].reset();
                }
            }
        }
        
        if((haveDepth && !_depthTexture[0]) || (haveDepth && _depthTexture[0]->getTexWidth() != width && _depthTexture[0]->getTexHeight() != height)){
            for(size_t i = 0; i < _depthTexture.size(); i++){
                if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_DEPTH){
                    std::string label = "Depth texture - Frame " + std::to_string(i);

                    IlenGPUTextureDescriptor descriptor{};
                    descriptor.height = height;
                    descriptor.width = width;
                    descriptor.presentation = enumerations::ILEN_GPU_TEXTURE_PRESENTATION_TYPE::ILEN_GPU_TEXTURE_PRESENTATION_NOTPRESENT;
                    descriptor.pixelFormat = enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH;
                    descriptor.usage = ILEN_GPU_TEXTURE_USAGE_RENDERTARGET;
                    descriptor.memoryProperty = enumerations::ILEN_GPU_MEMORY_PROPERTY_TYPE::ILEN_GPU_MEMORY_PROPERTY_PRIVATE;
                    descriptor.label = label;
                    
                    _depthTexture[i] = createNewPlatformTexture(descriptor);
                }else{
                    _depthTexture[i].reset();
                }
            }
        }

    }
}

IGPUTexture* MetalRender::getDrawableTexturePtr(){
    return _drawableTexture[_actualFrame].get();
}

std::vector<IGPUTexture*> MetalRender::getAllSceneTexturesPtr() {
    std::vector<IGPUTexture*> toReturn; //Vector a regresar
    size_t differentTextures = _sceneTexture.size() / MAX_FRAMES_IN_FLIGHT; //Numero de texturas diferentes
    toReturn.resize(differentTextures); //Optimización
    
    //Por cada textura diferente, mandamos al del frame actual
    for(size_t i = 0; i < differentTextures; i++){
        toReturn[i] = _sceneTexture[_actualFrame + (i * MAX_FRAMES_IN_FLIGHT)].get();
    }
    
    return toReturn;
}


std::unique_ptr<IGPUParallelCommandEncoder> MetalRender::createNewParallelCommandEncoder(IlenParallelEncoderDescriptor&& descriptor){
    //Parte de su responsabilidad es asignar las texturas para el nuevo Parallel Encoder Descriptor.
        //CAUTION: Cuidar todos los objetos que no retornan nada, porque pueden hacer crashear el programa de la nada, como al usar esto.
    return std::make_unique<MetalParallelCommandEncoder>(_commandBuffer[_actualFrame], std::move(descriptor));
}

std::unique_ptr<IGPUTexture> MetalRender::createNewPlatformTexture(const IlenGPUTextureDescriptor &descriptor){
    return std::make_unique<MetalTexture>(descriptor, _predefinedValues, _device);
}

void MetalRender::regenerateEncoderDescriptor(){
    IlenParallelEncoderDescriptor parallelEncoderDescriptor {};
    parallelEncoderDescriptor.label = "Main Render Pass Encoder";
    parallelEncoderDescriptor.colorAttachments.resize(3); //Metal soporta hasta 8, pero creo que de momento 3 son más que suficientes.
    auto colorAttach = std::make_unique<IlenEncoderAttachmentDescriptor>();

    if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS){
        colorAttach->texture = _sceneTexture[_actualFrame].get();
        uint32_t actualIndexForNextTexture = MAX_FRAMES_IN_FLIGHT;
        if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_MOTION_BLUR){
            //TODO: asignar la textura para el velocity buffer.
            auto motionBlurAttach = std::make_unique<IlenEncoderAttachmentDescriptor>();
            motionBlurAttach->texture = _sceneTexture[_actualFrame + actualIndexForNextTexture].get();
            motionBlurAttach->loadOption = ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_CLEAR;
            motionBlurAttach->storeOption = ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_STORE;
            //Porque este encoder no tiene asignado el attachment?? Luego así vienen los errores random.
            parallelEncoderDescriptor.colorAttachments[ilen::render::slots::RenderPassAttachment_VelocityBuffer] = std::move(motionBlurAttach); // Slot 1 para el Velocity Buffer
            actualIndexForNextTexture += 3; // Se añade un stride de 3 para los indices de alguna nueva textura especial a añadir.
            //motionBlurAttach->texture
        }
    }else{
        colorAttach->texture = _drawableTexture[_actualFrame].get();
    }
    colorAttach->loadOption = ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_CLEAR;
    colorAttach->storeOption = ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_STORE;
    colorAttach->clearColor = graphics::Color(_predefinedValues.clearColor.red, _predefinedValues.clearColor.green, _predefinedValues.clearColor.blue);
    parallelEncoderDescriptor.colorAttachments[ilen::render::slots::RenderPassAttachment_Color] = std::move(colorAttach);

    if(GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_DEPTH){
        //Depth buffer
        auto depthAttach = std::make_unique<IlenEncoderAttachmentDescriptor>();
        depthAttach->texture = _depthTexture[_actualFrame].get();
        depthAttach->loadOption = ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_CLEAR;
        depthAttach->storeOption = ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_DONT_CARE;
        
        parallelEncoderDescriptor.depthAttachment = std::move(depthAttach);
    }
    
    actualMainDescriptor = parallelEncoderDescriptor;
    lastUsedGraphicsOptions = GraphicsOptions::graphicsOptionsFlags;
}

std::unique_ptr<IGPUInstanceBuffer> MetalRender::createInstanceBuffer(size_t bufferSize, std::string bufferName){
    return std::make_unique<MetalInstanceBuffer>(_device, bufferSize, bufferName);
}

    /*
    Renderer* Renderer::create(void* native_surface){
        return new MetalRender(native_surface);
    }*/
}

#endif // defined (__APPLE__)

