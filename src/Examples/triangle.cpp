#include <ilen-examples/triangle.h>

using namespace ilen::game;
using namespace ilen::core;
using namespace ilen::ecs::components;

float advance = 0.0f;
bool isRight = true;

void Triangle::Init() {
    ILEN_LOG_P("TRIANGLE", "Inicializado triángulo de prueba");
    
    //Inicializar los sistemas (En el motor normal, al leer los datos de la escena, ahí los iniciaría en vez de esta forma)
    //coordinator->registerComponent<TransformComponent>();
    //coordinator->registerComponent<RenderComponent>();

    TransformComponent transform{};
    transform.position = vec3(0.0f, 0.0f, 0.0f);
    transform.scale = vec3(1.5f, 1.5f, 1.5f);
    coordinator->addComponent(entity, std::move(transform));

    RenderComponent render{};
    render.meshID = 2;
    render.mainTexID = 0;
    render.materialID = 5;
    coordinator->addComponent(entity, std::move(render)); 
}

void Triangle::Update(){
    using namespace ilen::engine::api;
//    ILEN_LOG_P("TRIANGLE", "Triangle update. Delta time: %.4f", ilen::time::Time::getDeltaTime());
    TransformAPI::Rotate(entity, vec3(0, 0, 360) * ilen::time::Time::getDeltaTime());

    float xAdvance = 10 * ilen::time::Time::getDeltaTime();
    if(advance > 5 || isRight == false) {
        xAdvance *= -1;
        isRight = false;
    }

    if(advance < -5){
        isRight = true;
    }

    TransformAPI::Translate(entity, vec3(xAdvance, 0, 0));
    advance += xAdvance;
}

void FalseCamera::Init() {
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
