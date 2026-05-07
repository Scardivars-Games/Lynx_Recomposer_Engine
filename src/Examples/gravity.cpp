#if !ILEN_ONLY_ECS

#include <ilen-examples/gravity.h>

using namespace ilen::game;
using namespace ilen::core;
using namespace ilen::ecs::components;
using namespace ilen::ecs::components::physics;


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
    render.mainTexID = 0;
    render.materialID = 5;
    coordinator->setComponent(entity, std::move(render)); 

    Rigidbody2D rigidbody{};
    rigidbody.gravityScale = 1.0f;
    coordinator->setComponent(entity, std::move(rigidbody));

    BoxCollider2D collider{};
    coordinator->setComponent(entity, std::move(collider));
}

void Ground::Init() {   
    //Inicializar los sistemas (En el motor normal, al leer los datos de la escena, ahí los iniciaría en vez de esta forma)
    //coordinator->registerComponent<TransformComponent>();
    //coordinator->registerComponent<RenderComponent>();

    TransformComponent transform{};
    transform.position = vec3(0.0f, -9.0f, 0.0f);
    transform.scale = vec3(90.0f, 0.5f, 1.0f);
    coordinator->setComponent(entity, std::move(transform));

    RenderComponent render{};
    render.meshID = 2;
    render.mainTexID = 0;
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
    camera.orthoSize = 20;
    camera.nearClip = 5;
    camera.projectionType = ILEN_CAMERA_PROJECTION_TYPE::ILEN_CAMERA_PROJECTION_ORTHOGRAPHIC;
    coordinator->setComponent(entity, std::move(camera));
}

#endif //!ILEN_ONLY_ECS