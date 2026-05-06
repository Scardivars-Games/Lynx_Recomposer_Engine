#if !ILEN_ONLY_ECS

#include <ilen-examples/triangle.h>

using namespace ilen::game;
using namespace ilen::core;
using namespace ilen::ecs::components;

void Triangle::Init() {
    //ILEN_LOG_P("TRIANGLE", "Inicializado triángulo de prueba");

    // Posición aleatoria entre -5 y 5 en X e Y
    float randomX = -35.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 70.0f);
    float randomY = -18.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 36.0f);
    TransformComponent transform{};
    transform.position = vec3(randomX, randomY, -1.0f);
    transform.scale = vec3(0.5f, 0.5f, 0.5f);
    coordinator->setComponent(entity, std::move(transform));

    RenderComponent render{};
    render.meshID = 2;
    render.mainTexID = 0;
    render.materialID = 5;
    coordinator->setComponent(entity, std::move(render));
}

void Triangle::Update(){
    using namespace ilen::engine::api;
//    ILEN_LOG_P("TRIANGLE", "Triangle update. Delta time: %.4f", ilen::time::Time::getDeltaTime());
    int zRotate = 360;

    float xAdvance = 5 * ilen::time::Time::getDeltaTime();
    if(advance > 5 || isRight == false) {
        zRotate = -360;
        xAdvance *= -1;
        isRight = false;
    }

    if(advance < -5){
        isRight = true;
    }

    TransformAPI::Rotate(entity, vec3(0, 0, static_cast<float>(zRotate)) * ilen::time::Time::getDeltaTime());
    TransformAPI::Translate(entity, vec3(xAdvance, 0, 0));
    advance += xAdvance;
}

void FalseCamera::Init() {
    TransformComponent transform{};
    transform.position = vec3(0.0f, 0.0f, -10.0f);
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    coordinator->setComponent(entity, std::move(transform));

    CameraComponent camera{};
    camera.isPrimary = true;
    camera.fovDegrees = 70.0f;
    //camera.nearClip = 5;
    camera.projectionType = ILEN_CAMERA_PROJECTION_TYPE::ILEN_CAMERA_PROJECTION_ORTHOGRAPHIC;
    coordinator->setComponent(entity, std::move(camera));
}

#endif //!ILEN_ONLY_ECS