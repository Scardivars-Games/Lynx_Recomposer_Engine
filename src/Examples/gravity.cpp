#include "ilen/core/time.h"
#if !ILEN_ONLY_ECS

#include <ilen_examples/gravity.h>
#include <ilen_public/API/input_api.h>
#include <ilen_public/API/physics_2d_api.h>
#include <ilen_public/API/entity_api.h>
#include <ilen_public/API/camera_api.h>
#include <ilen_public/API/transform_api.h>
#include <ilen_public/API/rendering_api.h>
#include <ilen_public/API/scenes_api.h>
#include <ilen/core/runtime_options.h>

#if ILEN_DEBUG
#include <ilen/debug/debug_render_system_2d.h>
#endif

using namespace ilen::game;
using namespace ilen::core;
using namespace ilen::ecs::components;
using namespace ilen::ecs::components::physics;
using namespace ilen::engine::api;
using namespace ilen::core::options;


int velocity = 15;
float deathZoneThreshold = 0.02f;
float deathZoneMobileSize = 0;
int lastWindowWidth = 0;

void Cube::Init() {
    ILEN_LOG_P("Gravedad", "Inicializado prueba de gravedad");

    deathZoneMobileSize = ilen::window::IWindow::getWidth() * deathZoneThreshold;
    lastWindowWidth = ilen::window::IWindow::getWidth();

    /*int elementosPrueba = 1000;

    for (int i = 0; i < elementosPrueba; i++) {
        EntityAPI::LoadPrefab("Bubble_Prefab");
    }*/

    //ScenesAPI::LoadSceneAdditiveAsync("ModelTest");
    
    //Inicializar los sistemas (En el motor normal, al leer los datos de la escena, ahí los iniciaría en vez de esta forma)
    //coordinator->registerComponent<TransformComponent>();
    //coordinator->registerComponent<RenderComponent>();

    /*TransformComponent transform{};
    transform.position = vec3(0.0f, 20.0f, 10.0f);
    transform.scale = vec3(3);
    coordinator->setComponent(entity, std::move(transform));

    RenderComponent render{};
    render.mainTexID = 15;
    coordinator->setComponent(entity, std::move(render)); 

    Rigidbody2D rigidbody{};
    rigidbody.gravityScale = 2.0f;
    rigidbody.mass = 0.5f;
    coordinator->setComponent(entity, std::move(rigidbody));

    BoxCollider2D collider{};
    collider.halfExtents = vec2(0.2f, 0.4f);
    coordinator->setComponent(entity, std::move(collider));*/

    //CameraAPI::MakeMainCameraFollowThisEntity(entity);
}

void Cube::Update() {
    using namespace ilen::engine::api;
    using namespace ilen::input;
    vec3 playerPos = TransformAPI::GetEntityPosition(entity);
    vec3 camPos = TransformAPI::GetEntityPosition(EntityAPI::GetEntityByName("Camara"));

    /*if(playerPos.x == camPos.x && playerPos.y == playerPos.y) {
        ILEN_LOG_P("TEST","Camara y Jugador comparten posición.");
    }*/

    if(InputAPI::IsKeyDown(Key::D) || InputAPI::IsKeyDown(Key::Right)){
        TransformAPI::Translate(entity, vec2(velocity, 0.0f) * time::Time::getDeltaTime());
        RenderingAPI::FlipSpriteInX(entity, false);
    }else if(InputAPI::IsKeyDown(Key::A) || InputAPI::IsKeyDown(Key::Left)){
        TransformAPI::Translate(entity, vec2(-velocity, 0.0f) * time::Time::getDeltaTime());
        RenderingAPI::FlipSpriteInX(entity, true);
    } else if(InputAPI::IsButtonDown(GamepadButton::LeftStick)) {
        float stickValue = InputAPI::GetGamepadValue(GamepadSpecialValues::LeftStickHorizontal);
        if(stickValue != 0){
            TransformAPI::Translate(entity, vec2(stickValue * velocity, 0.0f) * time::Time::getDeltaTime());
            RenderingAPI::FlipSpriteInX(entity, stickValue > 0 ? false : true);
        }

    }

    if(InputAPI::IsKeyPressed(Key::Space) || InputAPI::IsButtonPressed(GamepadButton::South)) {
        PhysicsAPI::AddImpulse(entity, vec2(0, 50));
    }

    auto touches = InputAPI::GetTotalTouches();
    //Movil táctil
    if(touches > 1) {
        //Hay touches!!!
        for(int i = 0; i < touches; i++) {
            if(InputAPI::IsTouchNew(i)){
                PhysicsAPI::AddImpulse(entity, vec2(0, 50));
                break;
            }
        }
    }

    //movimiento movil
    if(touches > 0) {
        //Hay touches!!!
        if(InputAPI::IsTouchDown(0)){
            //Indice 0, o sea, el primer touch echo en la app, o uno nuevo despues de liberar el Indice
            vec2 touchPos = InputAPI::GetTouchPosition(0);

            if(InputAPI::IsTouchNew(0)) {
                //Es el primer frame de este toque, almacenamos los datos.
                firstXPointerPos = touchPos.x;
            }

            if(lastWindowWidth != ilen::window::IWindow::getWidth()){
                deathZoneMobileSize = ilen::window::IWindow::getWidth() * deathZoneThreshold;
                lastWindowWidth = ilen::window::IWindow::getWidth();
            }
            
            //Vemos si debe ir para la derecha o la izquierda
            if((firstXPointerPos + deathZoneMobileSize) - touchPos.x < 0) {
                //Va para la derecha
                TransformAPI::Translate(entity, vec2(velocity, 0.0f) * time::Time::getDeltaTime());
                RenderingAPI::FlipSpriteInX(entity, false);
            }
            else if ((firstXPointerPos - deathZoneMobileSize) - touchPos.x > 0){
                //Va para la izquierda
                TransformAPI::Translate(entity, vec2(-velocity, 0.0f) * time::Time::getDeltaTime());
                RenderingAPI::FlipSpriteInX(entity, true);
            }
        }
    }

        //Debug, activar
#if ILEN_DEBUG
    using DebugSystem = ilen::ecs::systems::DebugRenderSystem2D;
    auto& debutSettings = DebugSystem::GetSettings();

    if(InputAPI::IsKeyPressed(Key::K)) {
        //Toglear las opciones
        debutSettings.showColliders = !debutSettings.showColliders;
    }
#endif
}


void PointerPos::Init() {
    /*TransformComponent transform{};
    transform.position = vec3(0.0f, 0.0f, 3.0f);
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    coordinator->setComponent(entity, std::move(transform));

    RenderComponent render{};
    render.mainTexID = 13;
    render.materialID = PredefinedAssetsID::DefaultUnlitTransparentMatID;
    render.defaultSpriteInTextureID = 10;
    coordinator->setComponent(entity, std::move(render)); 


    //API
    using namespace ilen::engine::api;
    EntityAPI::SetActive(entity, false);*/
}

void PointerPos::Update(){
    using namespace ilen::engine::api;
    //Escritorio
    if(InputAPI::IsPrimaryMouseButtonPressed()){
        EntityAPI::SetActive(entity, true);
        vec2 mousePos = InputAPI::GetMousePosition();
        vec3 worldPos = CameraAPI::ConvertScreenToWorld(CameraAPI::mainCamera, mousePos);
        worldPos.z = 0;
       // ILEN_LOG_P("AYUDA", vector::to_string(worldPos).c_str());
        TransformAPI::SetPosition(entity, worldPos);
    }else{
        EntityAPI::SetActive(entity, false);
    }

    //Movil
    if(InputAPI::GetTotalTouches() > 0) {
        //Hay touches!!!
        if(InputAPI::IsTouchDown(0)){
            //Indice 0, o sea, el primer touch echo en la app, o uno nuevo despues de liberar el Indice
            EntityAPI::SetActive(entity, true);
            vec2 touchPos = InputAPI::GetTouchPosition(0);
            vec3 worldPos = CameraAPI::ConvertScreenToWorld(CameraAPI::mainCamera, touchPos);
            worldPos.z = 0;
            //    ILEN_LOG_P("AYUDA", vector::to_string(touchPos).c_str());
            //ILEN_LOG_P("AYUDA", vector::to_string(worldPos).c_str());
            TransformAPI::SetPosition(entity, worldPos);
        }
    }
}


void Ground::Init() {   
    //EntityAPI::LoadPrefab("Player_Prefab");

    //Inicializar los sistemas (En el motor normal, al leer los datos de la escena, ahí los iniciaría en vez de esta forma)
    //coordinator->registerComponent<TransformComponent>();
    //coordinator->registerComponent<RenderComponent>();

   /* TransformComponent transform{};
    transform.position = vec3(0.0f, -6.7f, 100.0f);
    transform.scale = vec3(90.0f, 1.0f, 1.0f);
    coordinator->setComponent(entity, std::move(transform));

    RenderComponent render{};
    coordinator->setComponent(entity, std::move(render)); 

    Rigidbody2D rigidbody{};
    rigidbody.gravityScale = 1.0f;
    rigidbody.isStatic = true;
    coordinator->setComponent(entity, std::move(rigidbody));

    BoxCollider2D collider{};
    coordinator->setComponent(entity, std::move(collider));*/
}

void GravityCamera::Init() {
    /*TransformComponent transform{};
    transform.position = vec3(0.0f, 0.0f, -10.0f);
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    transform.rotation = vec3(0.0f, 0.0f, 0.0f);
    coordinator->setComponent(entity, std::move(transform));

    CameraComponent camera{};
    camera.isPrimary = true;
    camera.fovDegrees = 70.0f;
    camera.orthoSize = 15;
    camera.nearClip = 2;
    camera.projectionType = ILEN_CAMERA_PROJECTION_TYPE::ILEN_CAMERA_PROJECTION_ORTHOGRAPHIC;
    coordinator->setComponent(entity, std::move(camera));*/
}

void GravityCamera::LateUpdate() {
    //Obtener posicion de la camara
    vec3 playerPos = TransformAPI::GetEntityPosition(EntityAPI::GetEntityByName("Player"));
    vec3 actualPos = TransformAPI::GetEntityPosition(entity);
    vec3 newPos(playerPos.x, playerPos.y, actualPos.z);
    //ILEN_LOG_P("AYUDA", vector::to_string(newPos).c_str());
    TransformAPI::SetPosition(entity, newPos);
}

void PlayerSpawner::Init() {
    ILEN_LOG_P("Player Spawner", "Spwaneando al jugador");
    EntityAPI::LoadPrefab("Player_Prefab", TransformAPI::GetEntityPosition(entity));

#if ILEN_DEBUG
    using DebugSystem = ilen::ecs::systems::DebugRenderSystem2D;
    auto& debutSettings = DebugSystem::GetSettings();

    debutSettings.colliderMode = ilen::debug::DebugShapeMode2D::Solid;
    debutSettings.staticColor = ilen::graphics::Color{0.0f,0.5f,1.0f,1.0f};
    debutSettings.dynamicColor = ilen::graphics::Color{1.0f, 0.0f,0.0f, 1.0f};
    debutSettings.showColliders = false;
#endif
}

void RotateGround::Update() {
    TransformAPI::Rotate(entity, vec3(0,0,50) * ilen::time::Time::getDeltaTime());
}
#endif //!ILEN_ONLY_ECS