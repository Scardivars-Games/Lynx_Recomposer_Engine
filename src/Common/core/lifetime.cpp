#include <ilen/core/filemanager.h>
#include <ilen/core/lifetime.h>
#include <ilen/resources/assets-manager.h>
#include <ilen/headers/ecs-systems.h>
#include <ilen/job-system/job-system.h>


//FOR NO-EDITOR GAME
#include <ilen-game/triangle.h>
//END

using namespace ilen::core;

namespace ilen{
    AppRuntime& AppRuntime::getInstance(){
        static AppRuntime runtime;
        return runtime;
    }

    //Inicializa la aplicación!!!
    void AppRuntime::init(const application::ApplicationMetadata& metadata){
        engineContext = std::make_unique<EngineContext>();

        using namespace ilen::ecs;
        #if defined (ILEN_DEBUG)
        ILEN_WARNING_P(DEBUG_PREFIX, "App in Debug Mode");
        #endif
        //Inicializando elementos del motor.
        ilen::time::Time::Init();
        engineContext->p_jobSystem = std::make_unique<ilen::job_system::JobSystem>();
        engineContext->p_jobSystem->Initializate();

        //Inicializando la presentiación del motor.
        application::Application::setApplicationData(metadata);
        //Inicializando el administrador de archivos del motor.
        FileManager::Init();

        //Inicializar assets. Es lo más importante antes del resto de sistemas
        engineContext->p_assetsManager = std::make_unique<ilen::resources::AssetsManager>();
        engineContext->p_assetsManager->loadManifiest(engineContext.get());

        //Inicializando los sistemas del ECS
        engineContext->p_coordinator = std::make_unique<ilen::ecs::ECSCoordinator>();
        engineContext->p_coordinator->init();

        //Presentation
        using namespace ilen::window;
        using namespace ilen::render;
        //Crear ventana
        ILEN_LOG_P(DEBUG_PREFIX, "Creando la ventana");
        //pGameWindow = new GameWindow(&metadata);
        engineContext->p_gameWindow = IWindow::CreateWindow(metadata);

        ILEN_LOG_P(DEBUG_PREFIX, "Creando la superficie para el dibujo");
        engineContext->p_gameRenderer = IRenderer::CreateRenderer(engineContext->p_gameWindow->getSurface(), ILEN_GPU_GRAPHICS_API_TYPE::Auto);


        //Inicializar cada Resource Manager (se precisa del render para ciertas funciones con la GPU)
        engineContext->p_meshManager = std::make_unique<ilen::resources::mesh::MeshManager>(engineContext.get());
        engineContext->p_shaderManager = std::make_unique<ilen::resources::shaders::ShaderManager>(engineContext.get());
        engineContext->p_materialManager = std::make_unique<ilen::resources::materials::MaterialManager>(engineContext.get());
        engineContext->p_renderPipelineManager = std::make_unique<ilen::resources::render_pipeline::PipelineManager>(engineContext.get());

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

        //FOR NO-EDITOR-GAME
        //Init all object for no editor.
        auto triangle = engineContext->p_coordinator->createEntity();
        components::ScriptingBridgeComponent scripting{};
        scripting.bind<Triangle>(); //Indicamos que tipo de script corresponde a este scripting.
        engineContext->p_coordinator->addComponent(triangle, std::move(scripting));
        auto falseCamera = engineContext->p_coordinator->createEntity();
        components::ScriptingBridgeComponent bridge2{};
        bridge2.bind<FalseCamera>();
        engineContext->p_coordinator->addComponent(falseCamera, std::move(bridge2));

        
        engineContext->p_transformSystem->Init(engineContext.get());
        //Inicializamos los scripts.
        engineContext->p_scriptingSystem->Init(engineContext.get());
        //Se realiza la inicialización del renderizado.
        engineContext->p_renderSystem->Init(engineContext.get());
        //Inicializar cámara
        engineContext->p_cameraSystem->Init(engineContext.get());
        //RenderDraw System.
        engineContext->p_renderDrawSystem->Init(engineContext.get());
        ilen::time::Time::setNewFrame();
    }

    void AppRuntime::update() const {
        engineContext->p_gameWindow->processEvents();//Actualizamos eventos de la ventana o OS

        //ILEN_LOG_P(DEBUG_PREFIX, "Window size, %i x %i", engineContext->p_gameWindow->getWidth(), engineContext->p_gameWindow->getHeight());

        engineContext->p_scriptingSystem->Update(); //Hacemos todas las modificaciones que pida el códigos
        engineContext ->p_scriptingSystem->LateUpdate(); //Hacemos las modificaciones que deben hacerse tras el Update de todos los elementos del juegos

        engineContext->p_transformSystem->Update(); //Actualizamos las matrices de transformación de acuerdo a las instrucciones dejadas por el update (componentes de modificacion). y calculamos lo necesario para renderizar

        engineContext-> p_renderSystem -> Update(); //Preparamos y obtenemos los objetos que deben ser dibujados
        engineContext->p_cameraSystem ->Update(engineContext->p_gameWindow->getWidth(), engineContext->p_gameWindow->getHeight()); //Calculamos all de la camara

        if(engineContext->p_gameRenderer){
            engineContext->p_gameRenderer->beginFrame(); //Prepara la GPU para recibir comando de dibujo
            //Aquí deberia poblarse el queue de dibujado.
            engineContext->p_renderDrawSystem->Update();
            engineContext->p_gameRenderer->endFrame(); //Cierra el encoding y manda a dibujar
        }
        application::Application::stopUntilNextFrame();
        ilen::time::Time::setNewFrame();
    }

    void AppRuntime::shutdown() const {
        //Actualmente detiene el job system, que es el unico sistema que no se libera automaticamente por el OS.
        ILEN_LOG_P(DEBUG_PREFIX, "Shutting down engine");
        engineContext->p_jobSystem->StopJobSystem();
    }

    bool AppRuntime::isRunning() const {
        if(!engineContext->p_gameWindow) return false;
        return !engineContext->p_gameWindow -> shouldClose();
    }
}
