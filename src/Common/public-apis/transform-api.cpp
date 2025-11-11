#include <ilen-public/API/transform.h>

using namespace ilen::engine::api;
using namespace ilen::ecs::components;

void TransformAPI::Init(ilen::ecs::ECSCoordinator* coordinator){
    p_coordinator = coordinator;
}

void TransformAPI::CheckInit(){
    if(p_coordinator == nullptr){
        ILEN_EXECUTE_ERROR_P("TRANSFORM_API", "Not initilized Tranform API. Please initialize it from a central executable code, like the init of transform system");
    }
}

void TransformAPI::Translate(ilen::ecs::Entity entity, Vector3 traslation){
    CheckInit();
    Traslate_TransformOrder traslateComp{};
    traslateComp.traslation = traslation;

    p_coordinator->addComponent(entity, std::move(traslateComp));
}

void TransformAPI::Rotate(ilen::ecs::Entity entity, Vector3 rotation) {
    CheckInit();
    Rotation_TransformOrder rotationComp{};
    rotationComp.rotation = rotation;

    p_coordinator->addComponent(entity, std::move(rotationComp));
}

void TransformAPI::Rotate(ilen::ecs::Entity entity, Vector3 axis, float rotation) {
    CheckInit();
    Rotation_TransformOrder rotationComp{};
    rotationComp.rotation = axis * rotation;

    p_coordinator->addComponent(entity, std::move(rotationComp));
}

void TransformAPI::Scale(ilen::ecs::Entity entity, float scale){
    CheckInit();
    Scale_TransformOrder scaleComp{};
    scaleComp.scale = vec3(1,1,1) * scale;

    p_coordinator->addComponent(entity, std::move(scaleComp));
}