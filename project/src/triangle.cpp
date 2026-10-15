#include "lynx/headers/enumerations.h"
#if !LYNX_ONLY_ECS

#include <lynx_examples/triangle.h>
#include <lynx_public/API/transform_api.h>

using namespace lynx::game;
using namespace lynx::core;
using namespace lynx::ecs::components;

void Triangle::Init() {
    //LYNX_LOG_P("TRIANGLE", "Inicializado triángulo de prueba");

    // Posición aleatoria entre -5 y 5 en X e Y
    float randomX = -35.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 70.0f);
    float randomY = -18.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / 36.0f);
    /*TransformComponent transform{};
    transform.position = vec3(randomX, randomY, -1.0f);
    transform.scale = vec3(0.5f, 0.5f, 0.5f);
    coordinator->setComponent(entity, std::move(transform));*/

    using namespace lynx::engine::api;
    TransformAPI::SetPosition(entity, vec3(randomX, randomY, -1.0f));
    TransformAPI::Scale(entity, 0.5f);
}

void Triangle::Update(){
    using namespace lynx::engine::api;
//    LYNX_LOG_P("TRIANGLE", "Triangle update. Delta time: %.4f", lynx::time::Time::getDeltaTime());
    int zRotate = 360;

    float xAdvance = 5 * lynx::time::Time::getDeltaTime();
    if(advance > 5 || isRight == false) {
        zRotate = -360;
        xAdvance *= -1;
        isRight = false;
    }

    if(advance < -5){
        isRight = true;
    }

    TransformAPI::Rotate(entity, vec3(0, 0, static_cast<float>(zRotate)) * lynx::time::Time::getDeltaTime());
    TransformAPI::Translate(entity, vec3(xAdvance, 0, 0));
    advance += xAdvance;
}

void FalseCamera::Init() {
    TransformComponent transform{};
    transform.position = vec3(0.0f, 3.0f, -10.0f);
    transform.scale = vec3(1.0f, 1.0f, 1.0f);
    coordinator->setComponent(entity, std::move(transform));

    CameraComponent camera{};
    camera.isPrimary = true;
    camera.fovDegrees = 70.0f;
    camera.nearClip = 2;
    camera.projectionType = enumerations::LYNX_CAMERA_PROJECTION_TYPE::LYNX_CAMERA_PROJECTION_ORTHOGRAPHIC;
    coordinator->setComponent(entity, std::move(camera));
}

void Object3D::Init() {
    TransformComponent transform{};
    transform.position = vec3(0.0f, 0.0f, 0.0f);
    transform.scale = vec3(3.0f);
    coordinator->setComponent(entity, std::move(transform));

    RenderComponent render{};
    render.meshID = 17;
    render.mainTexID = 14;
    coordinator->setComponent(entity, std::move(render));
}

void Object3D::Update() {
    using namespace lynx::engine::api;
    TransformAPI::Rotate(entity, vec3(0, 50, 0) * lynx::time::Time::getDeltaTime());
    
}

#endif //!LYNX_ONLY_ECS