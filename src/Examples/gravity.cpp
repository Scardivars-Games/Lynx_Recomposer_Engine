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
    transform.position = vec3(0.0f, 0.0f, 0.0f);
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    coordinator->addComponent(entity, std::move(transform));

    RenderComponent render{};
    render.meshID = 2;
    render.mainTexID = 0;
    render.materialID = 5;
    coordinator->addComponent(entity, std::move(render)); 

    GravityComponent gravity{};
    gravity.gravityScale = 1.0f;
    coordinator->addComponent(entity, std::move(gravity));

    VelocityComponent velocity{};
    velocity.linearVelocity = vec3{0.0f,0.0f,0.0f};
    coordinator->addComponent(entity, std::move(velocity));
}

void GravityCamera::Init() {
    TransformComponent transform{};
    transform.position = vec3(0.0f, 0.0f, -10.0f);
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    coordinator->addComponent(entity, std::move(transform));

    CameraComponent camera{};
    camera.isPrimary = true;
    camera.fovDegrees = 70.0f;
    //camera.nearClip = 5;
    camera.projectionType = ILEN_CAMERA_PROJECTION_TYPE::ILEN_CAMERA_PROJECTION_ORTHOGRAPHIC;
    coordinator->addComponent(entity, std::move(camera));
}

#endif //!ILEN_ONLY_ECS