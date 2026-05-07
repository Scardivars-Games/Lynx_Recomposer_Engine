#include <ilen-public/API/physics-2d.h>

using namespace ilen::engine::api;
using namespace ilen::ecs;
using namespace components;

void Physics2DAPI::Init(const ilen::EngineContext* context){
    p_coordinator = context->p_coordinator.get();

}

void Physics2DAPI::GenerateBoxColliderSize(Entity e, physics::BoxCollider2D* collider){
    //Si el tamaño no es 0, el box collider ya existe y se respeta.
    if(collider->halfExtents.x != 0.0f && collider->halfExtents.y != 0.0f) return;

    auto* transform = p_coordinator->getComponent<TransformComponent>(e);

    if(!transform){
        //Si no hay transform, se le da una tamaño minimo de seguridad.
        collider->halfExtents = vec2(0.1f);
        collider->offset = vec2(0.0f);
        return;
    }

    //Obtenemos el tamaño del quad de acuerdo a su escala.
    float width = std::abs(transform->scale.x);
    float height = std::abs(transform->scale.y);

    collider->halfExtents.x = width * 0.5f;
    collider->halfExtents.y = height * 0.5f;

    collider->offset = vec2(0.0f);
}