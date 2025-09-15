#include <ilen/core/filemanager.h>
#include <ilen/core/lifetime.h>
#include <ilen/core/assets-manager.h>
#include <ilen/headers/ecs-systems.h>

//FOR NO-EDITOR GAME
#include <ilen-game/triangle.h>
//END

using namespace ilen::core;

std::unique_ptr<ilen::ecs::ECSCoordinator> gCoordinator;
std::shared_ptr<ilen::ecs::systems::RenderSystem> renderSystem;
std::shared_ptr<ilen::ecs::systems::TransformSystem> transformSystem;
std::shared_ptr<ilen::ecs::systems::ScriptingSystem> scriptingSystem;

namespace ilen{
    //Inicializa todo de la aplicación!!!
    void AppRuntime::init(application::ApplicationMetadata metadata){
        using namespace ilen::ecs;
        #if defined (ILEN_DEBUG)
        core::Debug::Warning("App in Debug Mode");
        #endif
        //Inicializando la presentiación del motor.
        application::Application::setApplicationData(metadata);
        //Inicializando el administrador de archivos del motor.
        FileManager::Init();

        //Inicializar assets. Es lo más importante antes del resto de sistemas
        ilen::core::AssetsManager::getInstance().loadManifiest("assets.json");
        
        //Inicializando elementos del motor.
        ilen::time::Time::Init();

        //Inicializando los sistemas del ECS
        gCoordinator = std::make_unique<ilen::ecs::ECSCoordinator>();
        gCoordinator->init();
        //Registros componentes
        gCoordinator->registerComponent<components::TransformComponent>();
        gCoordinator->registerComponent<components::RenderComponent>();
        gCoordinator->registerComponent<components::ScriptingBridgeComponent>();
        //Registro sistemas.
        //Transform system (space properties of an object)
        transformSystem = gCoordinator->registerSystem<systems::TransformSystem>();
        Signature transformSignature;
        transformSignature.set(gCoordinator->getComponentType<components::TransformComponent>());
        gCoordinator->setSystemSignature<systems::TransformSystem>(transformSignature);
        //Render System (render elements required to commit the command buffer in Platform Agnostic Renderer class)
        renderSystem = gCoordinator->registerSystem<systems::RenderSystem>();
        Signature renderSignature;
        renderSignature.set(gCoordinator->getComponentType<components::RenderComponent>());
        gCoordinator->setSystemSignature<systems::RenderSystem>(renderSignature);
        //TODO: Init ScriptSystem for gameobjects if don't work GameObjects here inherited classes.
        scriptingSystem = gCoordinator->registerSystem<systems::ScriptingSystem>();
        Signature scriptingSignature;
        scriptingSignature.set(gCoordinator->getComponentType<components::ScriptingBridgeComponent>());
        gCoordinator->setSystemSignature<systems::ScriptingSystem>(scriptingSignature);

        //FOR NO-EDITOR-GAME
        //Init all object for no editor.
        auto triangle = gCoordinator->createEntity();
        components::ScriptingBridgeComponent scripting{};
        scripting.bind<Triangle>(); //Indicamos que tipo de script corresponde a este scripting.
        gCoordinator->addComponent(triangle, scripting);

        //Presentation
        using namespace ilen::window;
        using namespace ilen::render;
        //Crear ventana
        Debug::Log("Creando la ventana", DEBUG_PREFIX);
        pGameWindow = new GameWindow(&metadata);

        Debug::Log("Creando la superficie para el dibujo", DEBUG_PREFIX);
        pGameRenderer = Renderer::create(pGameWindow->getSurface());
        
        //Inicializamos los scripts.
        scriptingSystem->init();
        ilen::time::Time::setNewFrame();
    }

    void AppRuntime::update(){
        #if !ILEN_IOS
        pGameWindow->processEvents();
        #endif

        scriptingSystem->update();
        scriptingSystem->lateUpdate();

        if(pGameRenderer){
            pGameRenderer->beginFrame();
            pGameRenderer->endFrame();
        }
        application::Application::stopUntilNextFrame();
        ilen::time::Time::setNewFrame();
    }

    void AppRuntime::shutdown(){
        //TODO
    }

    bool AppRuntime::isRunning() const {
        if(!pGameWindow) return false;
        return !pGameWindow -> shouldClose();
    }
}
