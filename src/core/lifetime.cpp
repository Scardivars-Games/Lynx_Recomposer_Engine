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

    //Inicializa todo de la aplicación!!!
    void AppRuntime::init(application::ApplicationMetadata metadata){
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
        engineContext->p_gameWindow = std::make_unique<GameWindow>(&metadata);

        ILEN_LOG_P(DEBUG_PREFIX, "Creando la superficie para el dibujo");
        engineContext->p_gameRenderer = std::unique_ptr<Renderer>(Renderer::create(engineContext->p_gameWindow->getSurface()));


        //Inicializar cada Resource Manager (se precisa del render para ciertas funciones con la GPU)
        engineContext->p_meshManager = std::make_unique<ilen::resources::mesh::MeshManager>(engineContext.get());

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

        //Init ScriptSystem for gameobjects if don't work GameObjects here inherited classes.
        engineContext->p_scriptingSystem = engineContext->p_coordinator->registerSystem<systems::ScriptingSystem>();
        Signature scriptingSignature;
        scriptingSignature.set(engineContext->p_coordinator->getComponentType<components::ScriptingBridgeComponent>());
        engineContext->p_coordinator->setSystemSignature<systems::ScriptingSystem>(scriptingSignature);

        //FOR NO-EDITOR-GAME
        //Init all object for no editor.
        auto triangle = engineContext->p_coordinator->createEntity();
        components::ScriptingBridgeComponent scripting{};
        scripting.bind<Triangle>(); //Indicamos que tipo de script corresponde a este scripting.
        engineContext->p_coordinator->addComponent(triangle, std::move(scripting));

        
        engineContext->p_transformSystem->Init(engineContext.get());
        //Inicializamos los scripts.
        engineContext->p_scriptingSystem->init(engineContext.get());
        //Se realiza la inicialización del renderizado.
        engineContext->p_renderSystem->Start(engineContext.get());
        ilen::time::Time::setNewFrame();
    }

    void AppRuntime::update(){

        #if !ILEN_IOS
        engineContext->p_gameWindow->processEvents();
        #endif

        engineContext->p_scriptingSystem->update();
        engineContext ->p_scriptingSystem->lateUpdate();

        engineContext-> p_renderSystem ->Update();

        if(engineContext->p_gameRenderer){
            engineContext->p_gameRenderer->beginFrame();
            engineContext->p_gameRenderer->endFrame();
        }
        application::Application::stopUntilNextFrame();
        ilen::time::Time::setNewFrame();
    }

    void AppRuntime::shutdown(){
        //TODO
        engineContext->p_jobSystem->StopJobSystem();
    }

    bool AppRuntime::isRunning() {
        if(!engineContext->p_gameWindow) return false;
        return !engineContext->p_gameWindow -> shouldClose();
    }
}
