#include <ilen/core/filemanager.h>
#include <ilen/core/lifetime.h>
#include <ilen/resources/assets-manager.h>
#include <ilen/headers/ecs-systems.h>
#include <ilen/job-system/job-system.h>
#include <ilen/render/post-process-effects/post-process-system.h>
#include <ilen/core/runtime_options.h>
#include <ilen/core/engine-context.h>
#include <ilen/ecs/system_registry.h>

//API públicas del motor
#include <ilen-public/API/input-api.h>

//FOR NO-EDITOR GAME
#include <ilen-examples/gravity.h>
#include <ilen-examples/triangle.h>
// END

using namespace ilen::core;

namespace ilen{
    AppRuntime& AppRuntime::getInstance(){
        static AppRuntime runtime;
        return runtime;
    }

    //Inicializa la aplicación!!!
    void AppRuntime::init(const application::ApplicationMetadata& metadata){
        //Establecer opciones gráficas por defecto
        IlenGraphicsOptionsFlags defaultOptions = 0;
        defaultOptions |= ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS;
        defaultOptions |= ILEN_GRAPHICS_OPTIONS_USE_MOTION_BLUR;
        
        ilen::core::options::GraphicsOptions::graphicsOptionsFlags = defaultOptions;


        engineContext = std::make_unique<EngineContext>();

        using namespace ilen::ecs;
        #if defined (ILEN_DEBUG)
        ILEN_WARNING_P(DEBUG_PREFIX, "App in Debug Mode");
        #endif
        //Inicializando elementos del motor.
        ilen::time::Time::Init();
        engineContext->p_jobSystem = std::make_unique<ilen::job_system::JobSystem>();
        engineContext->p_jobSystem->Initializate();
        engineContext->p_commandDispatcher = std::make_unique<ilen::job_system::CommandBufferDispatcher>();
        engineContext->p_commandDispatcher->Init(engineContext->p_jobSystem->AvailableThreads());

        //Inicializando la presentiación del motor.
        application::Application::setApplicationData(metadata);
        //Inicializando el administrador de archivos del motor.
        FileManager::Init();

        //Inicializar assets. Es lo más importante antes del resto de sistemas
        engineContext->p_assetsManager = std::make_unique<ilen::resources::AssetsManager>();
        engineContext->p_assetsManager->loadManifiest(engineContext.get());

        //Inicializando los sistemas del ECS
        engineContext->p_coordinator = std::make_unique<ilen::ecs::ECSCoordinator>();
        engineContext->p_coordinator->init(engineContext.get());

        //Presentation
        using namespace ilen::window;
        using namespace ilen::render;
        //Crear ventana
        ILEN_LOG_P(DEBUG_PREFIX, "Creando la ventana");
        //pGameWindow = new GameWindow(&metadata);
        engineContext->p_gameWindow = IWindow::CreateWindow(metadata);

#if !IGNORE_RENDERING
        ILEN_LOG_P(DEBUG_PREFIX, "Creando la superficie para el dibujo");
        engineContext->p_gameRenderer = IRenderer::CreateRenderer(engineContext->p_gameWindow->getSurface(), ILEN_GPU_GRAPHICS_API_TYPE::Auto);


        //Inicializar cada Resource Manager (se precisa del render para ciertas funciones con la GPU)
        engineContext->p_meshManager = std::make_unique<ilen::resources::mesh::MeshManager>(engineContext.get());
        engineContext->p_shaderManager = std::make_unique<ilen::resources::shaders::ShaderManager>(engineContext.get());
        engineContext->p_materialManager = std::make_unique<ilen::resources::materials::MaterialManager>(engineContext.get());
        engineContext->p_renderPipelineManager = std::make_unique<ilen::resources::render_pipeline::PipelineManager>(engineContext.get());

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
        ILEN_LOG_P(DEBUG_PREFIX, "REGISTRANDO SYSTEMAS NATIVOS DEL MOTOR");
        for(auto& registerCallback: ilen::ecs::SystemRegistry::GetRegistrators()){
            registerCallback(engineContext.get());
        }

#if !ILEN_ONLY_ECS
        //Guardamos una referencia al puntero de scripting
        scriptingSystemPtr = engineContext->p_coordinator->getSystem<ilen::ecs::systems::ScriptingSystem>();

        //FOR NO-EDITOR-GAME
        //Init all object for no editor.
        //RotateCube Test
        int elementosPrueba = 0;

        for (int i = 0; i < elementosPrueba; i++) {
                auto entity = engineContext->p_coordinator->createEntity();
                components::ScriptingBridgeComponent script{};
                script.bind<ilen::game::Triangle>();
                engineContext->p_coordinator->setComponent(entity, std::move(script));
        }

        //GravityTest :)
        {
                components::EntityMetadataComponent cubeMetadata {TagSystem::None, LayerSystem::Player, "Cubo de prueba"};
                auto cube_gravity = engineContext->p_coordinator->createEntity(std::move(cubeMetadata));
                components::ScriptingBridgeComponent cubeScript_Gravity{};
                cubeScript_Gravity.bind<ilen::game::Cube>(); //Indicamos que tipo de script corresponde a este scripting.
                engineContext->p_coordinator->setComponent(cube_gravity, std::move(cubeScript_Gravity));
        }
        {
                components::EntityMetadataComponent groundMetadata {TagSystem::Ground, LayerSystem::Default, "Tierra"};
                auto ground_gravity = engineContext->p_coordinator->createEntity(std::move(groundMetadata));
                components::ScriptingBridgeComponent groundScript_gravity{};
                groundScript_gravity.bind<ilen::game::Ground>(); //Indicamos que tipo de script corresponde a este scripting.
                engineContext->p_coordinator->setComponent(ground_gravity, std::move(groundScript_gravity));
        }
        {
                components::EntityMetadataComponent camMetadata {TagSystem::None, LayerSystem::None, "Camara"};
                auto camera_gravity = engineContext->p_coordinator->createEntity(std::move(camMetadata));
                components::ScriptingBridgeComponent cameraScript_gravity{};
                cameraScript_gravity.bind<ilen::game::GravityCamera>();
                engineContext->p_coordinator->setComponent(camera_gravity, std::move(cameraScript_gravity));
        }


#endif
        //El init de los script generar actualmente componentes, por eso es necesario que sea primero.
        scriptingSystemPtr->Init(engineContext.get());

        ILEN_LOG_P(DEBUG_PREFIX, "INICIALIZANDO SYSTEMAS NATIVOS DEL MOTOR");
        for(auto& system: engineContext->p_coordinator->getAllSystems()){
                if(system.get() == scriptingSystemPtr.get()) continue;
                system->Init(engineContext.get());
        }


        //Inicializar APIS del motor.
        ilen::engine::api::InputAPI::Init(engineContext.get());
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
    ILEN_EXECUTE_ERROR_P("TEST_ERROR_DIALOG", "This is a test dialog screen manually launched. Remove the target compile definition TEST_ERROR_WINDOW_AT_START to reanude normal app work");
#endif

        ilen::time::Time::setNewFrame();

#ifdef ILEN_DEBUG
        //auto data = engineContext->p_coordinator->getDebugSnapshot();
        //ILEN_WARNING_P("ECS DEBUG", data.toString().c_str());
#endif
    }

    void AppRuntime::update() const {
        ilen::time::Time::setNewFrame();

        //Gestionamos primero el tiempo para ejecutar el fixed update.
        static double accumulator = 0.0;
        constexpr const double fixedMethodTime = 1.0 / ilen::core::options::PhysicsOptions::PhysicsUpdateTime;

        double frameTime = ilen::time::Time::getDeltaTime();
        ILEN_WARNING("FPS: %.2f", 1.0/frameTime);

        if(frameTime > 0.25) frameTime = 0.25; //Esto es para evitar si hay lag, no calcular millones de pasos de física.

        accumulator += frameTime;

        //Procesamos eventos del OS
        engineContext->p_gameWindow->processEvents();

#if !IGNORE_RENDERING
        //Calculamos el FIXED Update
        {
                ilen::time::ScopedTimer timer("Fixed Update execution");
                 while(accumulator >= fixedMethodTime){
                        #if !ILEN_ONLY_ECS
                                if(scriptingSystemPtr) scriptingSystemPtr->FixedUpdate();
                        #endif //ILEN_ONLY_ECS
                        for(auto& system : engineContext->p_coordinator->getSystemsByPhase(ilen::ecs::SystemPhase::FixedUpdate)){
                                system->Update();
                        }

                        accumulator -= fixedMethodTime;
                }
        }
       
        {
                ilen::time::ScopedTimer timer("Update execution");
                //Calculos los objetos con un Update que no sean fijos en la pipeline
                for(auto& system : engineContext->p_coordinator->getSystemsByPhase(ilen::ecs::SystemPhase::VariableUpdate)){
                        system->Update();
                }

                //Late update del sistema de scripting
                #if !ILEN_ONLY_ECS
                        if(scriptingSystemPtr) scriptingSystemPtr->LateUpdate();
                #endif //ILEN_ONLY_ECS

                //Calculamos los updates de los datos necesarios antes de renderizar
                for(auto& system : engineContext->p_coordinator->getSystemsByPhase(ilen::ecs::SystemPhase::PreRender)){
                        system->Update();
                }
        }
        
        {
                ilen::time::ScopedTimer timer("Rendering execution");
                //Renderizado        
                if(engineContext->p_gameRenderer){
                        {
                                ilen::time::ScopedTimer timer("Begin frame time");
                                engineContext->p_gameRenderer->beginFrame(); //Prepara la GPU para recibir comando de dibujo
                        }
                        //Aquí deberia poblarse el queue de dibujado.
                        for(auto& system : engineContext->p_coordinator->getSystemsByPhase(ilen::ecs::SystemPhase::Render)){
                                system->Update();
                        }
                        {
                                ilen::time::ScopedTimer timer("End frame time");
                                engineContext->p_gameRenderer->endFrame(); //Cierra el encoding y manda a dibujar
                        }
                }
        }

#endif //IGNORE_RENDERING

#if !ILEN_IOS //Si estamos en iOS, no es necesario esto, creo?
        application::Application::stopUntilNextFrame();
#endif
        //Calculamos el FIXED Update
        /*while(accumulator >= fixedMethodTime){
            for(auto& system : engineContext->p_coordinator->getSyste)

            accumulator -= fixedMethodTime;
        }*/

        //OLD UPDATWE
        /*        engineContext->p_gameWindow->processEvents();//Actualizamos eventos de la ventana o OS

        //ILEN_LOG_P(DEBUG_PREFIX, "Window size, %i x %i", engineContext->p_gameWindow->getWidth(), engineContext->p_gameWindow->getHeight());
#if !IGNORE_RENDERING
        engineContext->p_scriptingSystem->Update(); //Hacemos todas las modificaciones que pida el códigos
        engineContext ->p_scriptingSystem->LateUpdate(); //Hacemos las modificaciones que deben hacerse tras el Update de todos los elementos del juegos

        engineContext->p_transformSystem->Update(); //Actualizamos las matrices de transformación de acuerdo a las instrucciones dejadas por el update (componentes de modificacion). y calculamos lo necesario para renderizar

        engineContext-> p_renderSystem -> Update(); //Preparamos y obtenemos los objetos que deben ser dibujados
        engineContext->p_cameraSystem ->Update(ilen::window::IWindow::getWidth(), ilen::window::IWindow::getHeight()); //Calculamos all de la camara

        if(engineContext->p_gameRenderer){
            engineContext->p_gameRenderer->beginFrame(); //Prepara la GPU para recibir comando de dibujo
            //Aquí deberia poblarse el queue de dibujado.
            engineContext->p_renderDrawSystem->Update();
            engineContext->p_postProcessSystem->Update();
            engineContext->p_gameRenderer->endFrame(); //Cierra el encoding y manda a dibujar
        }
#endif

#if !ILEN_IOS //Si estamos en iOS, no es necesario esto, creo?
        application::Application::stopUntilNextFrame();
#endif
        ilen::time::Time::setNewFrame();*/
    }

    void AppRuntime::shutdown() const {
        //Actualmente detiene el job system, que es el unico sistema que no se libera automaticamente por el OS.
        ILEN_LOG_P(DEBUG_PREFIX, "Shutting down engine");
#ifdef ILEN_DEBUG
        //auto data = engineContext->p_coordinator->getDebugSnapshot();
       // ILEN_WARNING_P("ECS DEBUG", data.toString().c_str());
#endif

        engineContext->p_jobSystem->StopJobSystem();
    }

    bool AppRuntime::isRunning() const {
        if(!engineContext->p_gameWindow) return false;
        return !engineContext->p_gameWindow -> shouldClose();
    }
}
