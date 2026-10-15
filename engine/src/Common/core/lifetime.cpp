#include <lynx/core/filemanager.h>
#include <lynx/core/lifetime.h>
#include <lynx/resources/assets_manager.h>
#include <lynx/headers/ecs_systems.h>
#include <lynx/job_system/job_system.h>
#include <lynx/render/post_process_effects/post_process_system.h>
//#include <lynx/core/runtime_options.h>
#include <lynx/core/engine_context.h>
#include <lynx/ecs/system_registry.h>
#include <lynx/resources/texture_manager.h>
#include <lynx/resources/scene_manager.h>

//API públicas del motor
#include <lynx_public/API/input_api.h>
#include <lynx_public/API/scenes_api.h>
#include <memory>
#include <lynx/core/runtime_configuration.h>
#include <utility>

//FOR NO-EDITOR GAME
//#include <lynx_examples/gravity.h>
//#include <lynx_examples/triangle.h>
//#include "lynx/headers/lynx_logs.h"
// END

using namespace lynx::core;

namespace lynx{
    AppRuntime& AppRuntime::getInstance(){
        static AppRuntime runtime;
        return runtime;
    }

    //Inicializa la aplicación!!!
    void AppRuntime::init(core::ProjectBootsrap bootsrap){
        //Establecer opciones gráficas por defecto
        /*LynxGraphicsOptionsFlags defaultOptions = 0;
        defaultOptions |= LYNX_GRAPHICS_OPTIONS_USE_POSTPROCESS;
        defaultOptions |= LYNX_GRAPHICS_OPTIONS_USE_MOTION_BLUR;
        defaultOptions |= LYNX_GRAPHICS_OPTIONS_USE_DEPTH;
        
        lynx::core::options::GraphicsOptions::graphicsOptionsFlags = defaultOptions;*/
        //Ahora las opciones se generan al compilar el proyecto, así que lo de arriba no es más necesario

        const auto metadata = std::move(bootsrap.metadata);
        const auto initialScene = bootsrap.initialScene;
        const auto registerReflection = bootsrap.registerReflection;

        engineContext = std::make_unique<EngineContext>();
        engineContext->runtimeConfiguration = std::make_unique<lynx::core::RuntimeConfiguration>(std::move(bootsrap.runtimeConfiguration));

        const auto& runtimeConfiguration = *engineContext->runtimeConfiguration;

        //Validacines básicas
        if(runtimeConfiguration.ecs.initialEntityCapacity == 0) {
        LYNX_EXECUTE_ERROR_P(
                DEBUG_PREFIX,
                "La capacidad inicial de entidades no puede ser cero"
                );
        }

        if(runtimeConfiguration.graphics.actualFPS == 0) {
                LYNX_EXECUTE_ERROR_P(
                DEBUG_PREFIX,
                "El framerate configurado no puede ser cero"
                );
        }

        if(runtimeConfiguration.graphics.instanceBufferSizeInMB == 0) {
                LYNX_EXECUTE_ERROR_P(
                DEBUG_PREFIX,
                "El tamaño del instance buffer no puede ser cero"
                );
        }

        if(runtimeConfiguration.graphics.generalBufferSizeInMB == 0) {
                LYNX_EXECUTE_ERROR_P(
                DEBUG_PREFIX,
                "El tamaño del buffer general no puede ser cero"
                );
        }

        if(runtimeConfiguration.physics.updatedPerSecond <= 0.0) {
                LYNX_EXECUTE_ERROR_P(
                DEBUG_PREFIX,
                "La frecuencia de actualización de físicas debe ser mayor que cero"
                );
        }

        using namespace lynx::ecs;
        #if defined (LYNX_DEBUG)
        LYNX_WARNING_P(DEBUG_PREFIX, "App in Debug Mode");
        #endif
        //Inicializando elementos del motor.
        lynx::time::Time::Init();
        engineContext->p_jobSystem = std::make_unique<lynx::job_system::JobSystem>();
        engineContext->p_jobSystem->Initializate();
        engineContext->p_commandDispatcher = std::make_unique<lynx::job_system::CommandBufferDispatcher>();
        engineContext->p_commandDispatcher->Init(engineContext->p_jobSystem->AvailableThreads(), engineContext->runtimeConfiguration->ecs.initialEntityCapacity);

        //Inicializando la presentiación del motor.
        application::Application::setApplicationData(metadata, engineContext.get());
        //Inicializando el administrador de archivos del motor.
        FileManager::Init();

        //Inicializar assets. Es lo más importante antes del resto de sistemas
        engineContext->p_assetsManager = std::make_unique<lynx::resources::AssetsManager>();
        engineContext->p_assetsManager->loadManifiest(engineContext.get());

        //Inicializando los sistemas del ECS
        engineContext->p_coordinator = std::make_unique<lynx::ecs::ECSCoordinator>();
        engineContext->p_coordinator->init(engineContext.get());

        //Presentation
        using namespace lynx::window;
        using namespace lynx::render;
        //Crear ventana
        LYNX_LOG_P(DEBUG_PREFIX, "Creando la ventana");
        //pGameWindow = new GameWindow(&metadata);
        engineContext->p_gameWindow = IWindow::CreateWindow(metadata);

#if !IGNORE_RENDERING
        LYNX_LOG_P(DEBUG_PREFIX, "Creando la superficie para el dibujo");
        engineContext->p_gameRenderer = IRenderer::CreateRenderer(engineContext->p_gameWindow->getSurface(), LYNX_GPU_GRAPHICS_API_TYPE::Auto);


        //Inicializar cada Resource Manager (se precisa del render para ciertas funciones con la GPU)
        engineContext->p_meshManager = std::make_unique<lynx::resources::mesh::MeshManager>(engineContext.get());
        engineContext->p_shaderManager = std::make_unique<lynx::resources::shaders::ShaderManager>(engineContext.get());
        engineContext->p_materialManager = std::make_unique<lynx::resources::materials::MaterialManager>(engineContext.get());
        engineContext->p_renderPipelineManager = std::make_unique<lynx::resources::render_pipeline::PipelineManager>(engineContext.get());
        engineContext->p_textureManager = std::make_unique<lynx::resources::texture::TextureManager>(engineContext.get());
        engineContext->p_sceneManager = std::make_unique<lynx::resources::scene::SceneManager>(engineContext.get());

        if(registerReflection == nullptr) {
        LYNX_EXECUTE_ERROR_P(
                DEBUG_PREFIX,
                "El proyecto no proporcionó el registro de reflexión"
        );
        }

        registerReflection(*engineContext->p_sceneManager);

        /*
        //Registro sistemas.
        //Transform system (space properties of an object)
        engineContext->p_transformSystem = engineContext->p_coordinator->registerSystem<systems::TransformSystem>();
        Signature transformSignature;
        transformSignature.set(engineContext->p_coordinator->getComponentType<components::TransformComponent>());
        engineContext->p_coordinator->setSystemSignature<systems::TransformSystem>(transformSignature);

        //Render System (render elements required to commit the command buffer in Platform Agnostic Renderer class)
        engineContext->p_renderSystem = engineContext->p_coordinator->registerSystem<systems::RenderSystem>();
        Signature renderSignature;
        renderSignature.set(engineContext->p_coordinator->getComponentType<components::RenderComponent>());
        engineContext->p_coordinator->setSystemSignature<systems::RenderSystem>(renderSignature);

        //Render Draw System (Llama a dibujar a la GPU según los datos generados por el Render System)
        engineContext->p_renderDrawSystem = engineContext->p_coordinator->registerSystem<systems::RenderDrawSystem>();
        Signature renderDrawSignature;
        //renderDrawSignature.set(engineContext->p_coordinator->getComponentType<components::RenderItem>()); //Esto no lo posee ninguna entidad como componente, no es firma válida.
        engineContext->p_coordinator->setSystemSignature<systems::RenderDrawSystem>(renderSignature); //Que reciba todas las entidades que el sistema anterior. TODO: Luego le añadimos firmas más específicas al funcionamiento del sistema.

        //Init ScriptSystem for gameobjects if don't work GameObjects here inherited classes.
        engineContext->p_scriptingSystem = engineContext->p_coordinator->registerSystem<systems::ScriptingSystem>();
        Signature scriptingSignature;
        scriptingSignature.set(engineContext->p_coordinator->getComponentType<components::ScriptingBridgeComponent>());
        engineContext->p_coordinator->setSystemSignature<systems::ScriptingSystem>(scriptingSignature);

        //Camera system
        engineContext->p_cameraSystem = engineContext->p_coordinator->registerSystem<systems::CameraSystem>();
        Signature cameraSignature;
        cameraSignature.set(engineContext->p_coordinator->getComponentType<components::CameraComponent>());
        engineContext->p_coordinator->setSystemSignature<systems::CameraSystem>(cameraSignature);

        //Post processing system
        engineContext->p_postProcessSystem = engineContext->p_coordinator->registerSystem<systems::PostProcessSystem>();
        engineContext->p_coordinator->setSystemSignature<systems::PostProcessSystem>(renderSignature); //Comparte firma con el sistema de renderizado :)
*/
        LYNX_LOG_P(DEBUG_PREFIX, "REGISTRANDO SYSTEMAS NATIVOS DEL MOTOR");
        for(auto& registerCallback: lynx::ecs::SystemRegistry::GetRegistrators()){
            registerCallback(engineContext.get());
        }

        engineContext->p_coordinator->sortSystems();

#if !LYNX_ONLY_ECS
        //Guardamos una referencia al puntero de scripting
        scriptingSystemPtr = engineContext->p_coordinator->getSystem<lynx::ecs::systems::ScriptingSystem>();

        //Tras inicializar la app, se llamara a la API de escena y se cargara automaticamente la escena con el índice 0.

        //FOR NO-EDITOR-GAME
        //Init all object for no editor.
        //RotateCube Test
        /*int elementosPrueba = 1000;

        for (int i = 0; i < elementosPrueba; i++) {
                auto entity = engineContext->p_coordinator->createEntity();
                components::ScriptingBridgeComponent script{};
                script.bind<lynx::game::Triangle>();
                engineContext->p_coordinator->setComponent(entity, std::move(script));
        }*/

        //GravityTest :)
        /*{
                components::EntityMetadataComponent cubeMetadata {TagSystem::None, LayerSystem::Player, "Cubo de prueba"};
                auto cube_gravity = engineContext->p_coordinator->createEntity(std::move(cubeMetadata));
                components::ScriptingBridgeComponent cubeScript_Gravity{};
                cubeScript_Gravity.bind<lynx::game::Cube>(); //Indicamos que tipo de script corresponde a este scripting.
                engineContext->p_coordinator->setComponent(cube_gravity, std::move(cubeScript_Gravity));
        }
        {
                components::EntityMetadataComponent groundMetadata {TagSystem::Ground, LayerSystem::Default, "Tierra"};
                auto ground_gravity = engineContext->p_coordinator->createEntity(std::move(groundMetadata));
                components::ScriptingBridgeComponent groundScript_gravity{};
                groundScript_gravity.bind<lynx::game::Ground>(); //Indicamos que tipo de script corresponde a este scripting.
                engineContext->p_coordinator->setComponent(ground_gravity, std::move(groundScript_gravity));
        }
        {
                components::EntityMetadataComponent camMetadata {TagSystem::None, LayerSystem::None, "Camara"};
                auto camera_gravity = engineContext->p_coordinator->createEntity(std::move(camMetadata));
                components::ScriptingBridgeComponent cameraScript_gravity{};
                cameraScript_gravity.bind<lynx::game::GravityCamera>();
                engineContext->p_coordinator->setComponent(camera_gravity, std::move(cameraScript_gravity));
        }
        {
                components::EntityMetadataComponent pointerMetadata {TagSystem::None, LayerSystem::UI, "Pointer Pos"};
                auto pointer = engineContext->p_coordinator->createEntity(std::move(pointerMetadata));
                components::ScriptingBridgeComponent pointer_script{};
                pointer_script.bind<lynx::game::PointerPos>();
                engineContext->p_coordinator->setComponent(pointer, std::move(pointer_script));
        }
        {
                components::EntityMetadataComponent model3D {TagSystem::None, LayerSystem::None, "Cash Register"};
                auto model3DEntity = engineContext->p_coordinator->createEntity(std::move(model3D));
                components::ScriptingBridgeComponent model3D_script{};
                model3D_script.bind<lynx::game::Object3D>();
                engineContext->p_coordinator->setComponent(model3DEntity, std::move(model3D_script));
        }*/


#endif

        //Inicializar APIS del motor.
        lynx::engine::api::InputAPI::Init(engineContext.get());
        lynx::engine::api::ScenesAPI::Init(engineContext.get());

#if !LYNX_ONLY_ECS
        //El init de los script generar actualmente componentes, por eso es necesario que sea primero.
        scriptingSystemPtr->Init(engineContext.get());
#endif
        //Cargar escena de indice 1 (0 siempre significa prefab)
        if(initialScene != 0) {
                engineContext->p_sceneManager->LoadScene(initialScene);
        } else {
                LYNX_WARNING_P(
                        DEBUG_PREFIX,
                        "El proyecto no definió una escena inicial"
                );
        }

        LYNX_LOG_P(DEBUG_PREFIX, "INICIALIZANDO SYSTEMAS NATIVOS DEL MOTOR");
        for(auto& system: engineContext->p_coordinator->getAllSystems()){
#if !LYNX_ONLY_ECS
                if(system.get() == scriptingSystemPtr.get()) continue;
#endif
                system->Init(engineContext.get());
        }

        /*
        engineContext->p_transformSystem->Init(engineContext.get());
        //Inicializamos los scripts.
        engineContext->p_scriptingSystem->Init(engineContext.get());
        //Se realiza la inicialización del renderizado.
        engineContext->p_renderSystem->Init(engineContext.get());
        //Inicializar cámara
        engineContext->p_cameraSystem->Init(engineContext.get());
        //RenderDraw System.
        engineContext->p_renderDrawSystem->Init(engineContext.get());
        //Post procesad
        engineContext->p_postProcessSystem->Init(engineContext.get());*/
#endif

#if TEST_ERROR_WINDOW_AT_START
    LYNX_EXECUTE_ERROR_P("TEST_ERROR_DIALOG", "This is a test dialog screen manually launched. Remove the target compile definition TEST_ERROR_WINDOW_AT_START to reanude normal app work");
#endif

        lynx::time::Time::setNewFrame();

#ifdef LYNX_DEBUG
        auto data = engineContext->p_coordinator->getDebugSnapshot();
        LYNX_WARNING_P("ECS DEBUG", data.toString().c_str());
#endif
    }

    void AppRuntime::update() const {
        lynx::time::Time::setNewFrame();

        //Gestionamos primero el tiempo para ejecutar el fixed update.
        static double accumulator = 0.0;
        //const double fixedMethodTime = 1.0 / lynx::core::options::PhysicsOptions::PhysicsUpdateTime;
        const double fixedDeltaSeconds = 1.0 / engineContext->runtimeConfiguration->physics.updatedPerSecond;
        
        double frameTime = lynx::time::Time::getDeltaTime();
        //LYNX_WARNING("FPS: %.2f", 1.0/frameTime);

        if(frameTime > 0.25) frameTime = 0.25; //Esto es para evitar si hay lag, no calcular millones de pasos de física.

        accumulator += frameTime;

        //Procesamos eventos del OS
        engineContext->p_gameWindow->processEvents();
        engineContext->p_gameWindow->updateGamepads();


        if(!engineContext->p_gameWindow->shouldPause()){
        //LYNX_WARNING("FPS: %.2f", 1.0/frameTime);
#if !IGNORE_RENDERING
            {
                lynx::time::ScopedTimer timer("Game CPU Time");
                // Aplicar cargas asíncronas completas antes de ejecutar sistemas del ECS.
                engineContext->p_sceneManager->Update(static_cast<float>(frameTime));
                //Calculamos el FIXED Update
                {
                        lynx::time::ScopedTimer timer("Fixed Update execution");
                        while(accumulator >= fixedDeltaSeconds){
                                #if !LYNX_ONLY_ECS
                                        if(scriptingSystemPtr) scriptingSystemPtr->FixedUpdate();
                                #endif //LYNX_ONLY_ECS
                                for(auto& system : engineContext->p_coordinator->getSystemsByPhase(lynx::ecs::SystemPhase::FixedUpdate)){
                                        system->Update();
                                }

                                accumulator -= fixedDeltaSeconds;
                        }
                }
        
                {
                        lynx::time::ScopedTimer timer("Update execution");
                        //Calculos los objetos con un Update que no sean fijos en la pipeline
                        for(auto& system : engineContext->p_coordinator->getSystemsByPhase(lynx::ecs::SystemPhase::VariableUpdate)){
                                system->Update();
                        }

                        //Late update del sistema de scripting
                        #if !LYNX_ONLY_ECS
                                if(scriptingSystemPtr) scriptingSystemPtr->LateUpdate();
                        #endif //LYNX_ONLY_ECS

                        //Calculamos los updates de los datos necesarios antes de renderizar
                        for(auto& system : engineContext->p_coordinator->getSystemsByPhase(lynx::ecs::SystemPhase::PreRender)){
                                system->Update();
                        }
                }
                
                {
                        lynx::time::ScopedTimer timer("Rendering execution");
                        //Renderizado
                        if(engineContext->p_gameRenderer){
                                {
                                        lynx::time::ScopedTimer timer("Begin frame time");
                                        engineContext->p_gameRenderer->beginFrame(); //Prepara la GPU para recibir comando de dibujo
                                }
                                // Subir uniformes solo después de adquirir el slot de GPU.
                                auto cameraSystem = engineContext->p_coordinator->getSystem<lynx::ecs::systems::CameraSystem>();
                                if (cameraSystem) cameraSystem->UploadFrameData();
                                //Aquí deberia poblarse el queue de dibujado.
                                for(auto& system : engineContext->p_coordinator->getSystemsByPhase(lynx::ecs::SystemPhase::Render)){
                                        system->Update();
                                }
                                {
                                        lynx::time::ScopedTimer timer("End frame time");
                                        engineContext->p_gameRenderer->endFrame(); //Cierra el encoding y manda a dibujar
                                }
                        }
                }
            }

#endif //IGNORE_RENDERING
        }

        //Preparamos el input para los eventos en el proximo frame (se coloca aquí porque hay eventos que llegan cuando quieren como en iOS, lo cual dificultaria la logica de numerosas funciones del sistema para comprobar si es nuevo o no):
        lynx::input::InputBuffer::Get().NewFrame();
#if !LYNX_IOS && !LYNX_TVOS //Si estamos en iOS, no es necesario esto, creo?
        //LYNX_LOG_P("TEST", "En iOS esto no debe aparecer");
        application::Application::stopUntilNextFrame();
#endif


        //Calculamos el FIXED Update
        /*while(accumulator >= fixedMethodTime){
            for(auto& system : engineContext->p_coordinator->getSyste)

            accumulator -= fixedMethodTime;
        }*/

        //OLD UPDATWE
        /*        engineContext->p_gameWindow->processEvents();//Actualizamos eventos de la ventana o OS

        //LYNX_LOG_P(DEBUG_PREFIX, "Window size, %i x %i", engineContext->p_gameWindow->getWidth(), engineContext->p_gameWindow->getHeight());
#if !IGNORE_RENDERING
        engineContext->p_scriptingSystem->Update(); //Hacemos todas las modificaciones que pida el códigos
        engineContext ->p_scriptingSystem->LateUpdate(); //Hacemos las modificaciones que deben hacerse tras el Update de todos los elementos del juegos

        engineContext->p_transformSystem->Update(); //Actualizamos las matrices de transformación de acuerdo a las instrucciones dejadas por el update (componentes de modificacion). y calculamos lo necesario para renderizar

        engineContext-> p_renderSystem -> Update(); //Preparamos y obtenemos los objetos que deben ser dibujados
        engineContext->p_cameraSystem ->Update(lynx::window::IWindow::getWidth(), lynx::window::IWindow::getHeight()); //Calculamos all de la camara

        if(engineContext->p_gameRenderer){
            engineContext->p_gameRenderer->beginFrame(); //Prepara la GPU para recibir comando de dibujo
            //Aquí deberia poblarse el queue de dibujado.
            engineContext->p_renderDrawSystem->Update();
            engineContext->p_postProcessSystem->Update();
            engineContext->p_gameRenderer->endFrame(); //Cierra el encoding y manda a dibujar
        }
#endif

#if !LYNX_IOS //Si estamos en iOS, no es necesario esto, creo?
        application::Application::stopUntilNextFrame();
#endif
        lynx::time::Time::setNewFrame();*/
    }

    void AppRuntime::shutdown() const {
        //Actualmente detiene el job system, que es el unico sistema que no se libera automaticamente por el OS.
        LYNX_LOG_P(DEBUG_PREFIX, "Shutting down engine");
#ifdef LYNX_DEBUG
        //auto data = engineContext->p_coordinator->getDebugSnapshot();
       // LYNX_WARNING_P("ECS DEBUG", data.toString().c_str());
#endif

        engineContext->p_jobSystem->StopJobSystem();
    }

    bool AppRuntime::isRunning() const {
        if(!engineContext->p_gameWindow) return false;
        return !engineContext->p_gameWindow -> shouldClose();
    }
}
