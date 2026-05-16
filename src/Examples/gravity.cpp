#if !ILEN_ONLY_ECS

#include <ilen-examples/gravity.h>
#include <ilen-public/API/input-api.h>
#include <ilen-public/API/physics-2d.h>
#include <ilen-public/API/entity-api.h>
#include <ilen-public/API/camera-api.h>

using namespace ilen::game;
using namespace ilen::core;
using namespace ilen::ecs::components;
using namespace ilen::ecs::components::physics;
using namespace ilen::engine::api;

void Cube::Init() {
    ILEN_LOG_P("Gravedad", "Inicializado prueba de gravedad");
    
    //Inicializar los sistemas (En el motor normal, al leer los datos de la escena, ahí los iniciaría en vez de esta forma)
    //coordinator->registerComponent<TransformComponent>();
    //coordinator->registerComponent<RenderComponent>();

    TransformComponent transform{};
    transform.position = vec3(0.0f, 20.0f, 0.0f);
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    coordinator->setComponent(entity, std::move(transform));

    RenderComponent render{};
    render.meshID = 2;
    render.mainTexID = 14;
    render.materialID = 5;
    coordinator->setComponent(entity, std::move(render)); 

    Rigidbody2D rigidbody{};
    rigidbody.gravityScale = 2.0f;
    rigidbody.mass = 0.5f;
    coordinator->setComponent(entity, std::move(rigidbody));

    BoxCollider2D collider{};
    coordinator->setComponent(entity, std::move(collider));

    //CameraAPI::MakeMainCameraFollowThisEntity(entity);
}

int velocity = 15;

void Cube::Update() {
    using namespace ilen::engine::api;
    using namespace ilen::input;
    if(InputAPI::IsKeyDown(Key::D) || InputAPI::IsKeyDown(Key::Right)){
        TransformAPI::Translate(entity, vec2(velocity, 0.0f) * time::Time::getDeltaTime());
    }else if(InputAPI::IsKeyDown(Key::A) || InputAPI::IsKeyDown(Key::Left)){
        TransformAPI::Translate(entity, vec2(-velocity, 0.0f) * time::Time::getDeltaTime());
    } else if(InputAPI::IsButtonDown(GamepadButton::LeftStick)) {
        TransformAPI::Translate(entity, vec2(InputAPI::GetGamepadValue(GamepadSpecialValues::LeftStickHorizontal) * velocity, 0.0f) * time::Time::getDeltaTime());
    }

    if(InputAPI::IsKeyPressed(Key::Space) || InputAPI::IsButtonPressed(GamepadButton::South)) {
        PhysicsAPI::AddImpulse(entity, vec2(0, 100));
    }

    auto touches = InputAPI::GetTotalTouches();
    //Movil táctil
    if(touches > 1) {
        //Hay touches!!!
        for(int i = 0; i < touches; i++) {
            if(InputAPI::IsTouchNew(i)){
                PhysicsAPI::AddImpulse(entity, vec2(0, 100));
                break;
            }
        }
    }
}


void PointerPos::Init() {
    TransformComponent transform{};
    transform.position = vec3(0.0f, 0.0f, 0.0f);
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    coordinator->setComponent(entity, std::move(transform));

    RenderComponent render{};
    render.meshID = 1;
    render.materialID = 5;
    coordinator->setComponent(entity, std::move(render)); 


    //API
    using namespace ilen::engine::api;
    EntityAPI::SetActive(entity, false);
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
    //Inicializar los sistemas (En el motor normal, al leer los datos de la escena, ahí los iniciaría en vez de esta forma)
    //coordinator->registerComponent<TransformComponent>();
    //coordinator->registerComponent<RenderComponent>();

    TransformComponent transform{};
    transform.position = vec3(0.0f, -6.7f, 0.0f);
    transform.scale = vec3(90.0f, 1.0f, 1.0f);
    coordinator->setComponent(entity, std::move(transform));

    RenderComponent render{};
    render.meshID = 2;
    render.materialID = 5;
    coordinator->setComponent(entity, std::move(render)); 

    Rigidbody2D rigidbody{};
    rigidbody.gravityScale = 1.0f;
    rigidbody.isStatic = true;
    coordinator->setComponent(entity, std::move(rigidbody));

    BoxCollider2D collider{};
    coordinator->setComponent(entity, std::move(collider));
}

void GravityCamera::Init() {
    TransformComponent transform{};
    transform.position = vec3(0.0f, 0.0f, -10.0f);
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    coordinator->setComponent(entity, std::move(transform));

    CameraComponent camera{};
    camera.isPrimary = true;
    camera.fovDegrees = 70.0f;
    camera.orthoSize = 15;
    camera.nearClip = 5;
    camera.projectionType = ILEN_CAMERA_PROJECTION_TYPE::ILEN_CAMERA_PROJECTION_ORTHOGRAPHIC;
    coordinator->setComponent(entity, std::move(camera));
}

void GravityCamera::LateUpdate() {
    //Obtener posicion de la camara
    vec3 playerPos = TransformAPI::GetEntityPosition(EntityAPI::GetEntityByName("Cubo de prueba"));
    vec3 actualPos = TransformAPI::GetEntityPosition(entity);
    vec3 newPos(playerPos.x, actualPos.y, actualPos.z);
    ILEN_LOG_P("AYUDA", vector::to_string(newPos).c_str());
    TransformAPI::SetPosition(entity, newPos);
}

#endif //!ILEN_ONLY_ECS