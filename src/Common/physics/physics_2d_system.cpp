#include <algorithm>
#include <ilen/physics/physics_2d_system.h>
#include <ilen/headers/ecs.h>
#include <ilen/headers/ecs_components.h>
#include <ilen_public/API/physics_2d_api.h>
#include <iterator>
#include <optional>
#include <utility>


using namespace ilen::ecs::systems;
using namespace ilen::ecs::components;
using namespace ilen::maths::vector;

void PhysicsSystem2D::Init(const ilen::EngineContext* engineContext){
    p_coordinator = engineContext->p_coordinator.get();
    p_jobSystem = engineContext->p_jobSystem.get();
    p_commandDispatcher = engineContext->p_commandDispatcher.get();

    m_dynamicEntities.reserve(MAX_ENTITIES);
    m_staticProxies.reserve(MAX_ENTITIES);
    m_dynamicProxies.reserve(MAX_ENTITIES);
    
#if ILEN_DEBUG
    m_staticGuards.reserve(MAX_ENTITIES);
#endif

    p_coordinator->registerComponent<physics::Rigidbody2D>();
    p_coordinator->registerComponent<physics::BoxCollider2D>();
    p_coordinator->registerComponent<physics::ConvexPolygonCollider2D>();
    p_coordinator->registerComponent<physics::CollisionComponent2D>();

    ilen::engine::api::PhysicsAPI::Init(engineContext);
}


//TODO: hacer este sistema multihilo.
void PhysicsSystem2D::Update() {
    ilen::time::ScopedTimer timer("Physics 2D update");
   
    ArrayOfComponentsUsed componentArray{};
    
    componentArray.rigidbodyComponentArray = p_coordinator->getComponentArray<physics::Rigidbody2D>();
    componentArray.collisionComponentArray = p_coordinator->getComponentArray<physics::CollisionComponent2D>();
    componentArray.boxColliderArray = p_coordinator->getComponentArray<physics::BoxCollider2D>();
    componentArray.transformComponentArray = p_coordinator->getComponentArray<TransformComponent>();
    componentArray.physicsOrderArray = p_coordinator->getComponentArray<physics::PhysicsOrder>();
    componentArray.transformOrderArray = p_coordinator->getComponentArray<TransformOrder>();
    componentArray.entityMetadataArray = p_coordinator->getComponentArray<EntityMetadataComponent>();
    componentArray.convexPolygonCollider = p_coordinator->getComponentArray<physics::ConvexPolygonCollider2D>();

    m_accumulatedTransformOrder.clear();


    syncEntities(&componentArray);

    //if(m_dynamicEntities.size() + m_staticEntities.size() != entities.size())
    if(m_dynamicEntities.empty() && m_staticProxies.empty()) return;

    constexpr float dt = 1.0f / ilen::core::options::PhysicsOptions::PhysicsUpdateTime;

    clearCollisionData(&componentArray);
    registerPrevPosition(&componentArray);
    applyForces(dt, &componentArray);
    applyGravity(dt, &componentArray);
    integrate(dt, &componentArray);
    rebuildDynamicProxies(dt, &componentArray);
    detectAndResolve(&componentArray);

    for(auto& [id, order] : m_accumulatedTransformOrder) {
        order.hasOrders = true;
        componentArray.transformOrderArray->updateData(id, std::move(order));
    }

#if ILEN_DEBUG
    checkStaticGuard(&componentArray);
#endif
}

void PhysicsSystem2D::registerEntity(Entity e, ArrayOfComponentsUsed* componentsArray) {
    auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
    auto* boxColider = componentsArray->boxColliderArray->getData(e);
    auto* polygonCollider = componentsArray->convexPolygonCollider->getData(e);

    if(!rb) return;

    if((boxColider == nullptr) == (polygonCollider == nullptr)) {
        ILEN_ERROR_P("PHYSICS_2D", "La entidad %u debe tener al menos un collider 2D", e);
        return;
    }

    //Método de seguridad para forzar el collider si este no existe.
    if(boxColider && boxColider->halfExtents.x == 0.0f && boxColider->halfExtents.y == 0.0f) {
        ilen::engine::api::PhysicsAPI::GenerateBoxColliderSize(e, boxColider);
    }

    auto proxy = buildProxy(e, componentsArray);

    if(!proxy.has_value()) {
        return;
    }

    if(rb->isStatic) {
        m_staticProxies.push_back(std::move(*proxy));

        m_registrationStates[e] = RegistrationState::Static;
#if ILEN_DEBUG
        auto* transform = p_coordinator->getComponent<TransformComponent>(e);
        m_staticGuards.push_back({
            .entity = e,
            .originalPosition = transform->position,
            .originalRotation = transform->rotation,
            .originalScale = transform->scale,
            .frameCounter = 0,
            .checkInterval = 15
        });
#endif
        return;
    }

    m_dynamicEntities.push_back(e);
    m_registrationStates[e] = RegistrationState::Dynamic;
}

void PhysicsSystem2D::syncEntities(ArrayOfComponentsUsed* componentsArray) {
    //Se encarga de eliminar y registrar las entidades con colliders para calcularse
    for(int i = static_cast<int>(m_dynamicEntities.size()) - 1; i >= 0; i--) {
        const Entity entity = m_dynamicEntities[i];

        if(entities.find(entity) != entities.end()){
            continue; //Existe
        }

        m_registrationStates[entity] = RegistrationState::None;
        m_dynamicEntities[i] = m_dynamicEntities.back();
        m_dynamicEntities.pop_back();
    } 

    for(int i = static_cast<int>(m_staticProxies.size()) - 1; i >= 0; i--){ 
        const Entity entity = m_staticProxies[i].entity;

        if(entities.find(entity) != entities.end()) {
            continue;
        }

        m_registrationStates[entity] = RegistrationState::None;
        m_staticProxies[i] = std::move(m_staticProxies.back());
        m_staticProxies.pop_back();
#if ILEN_DEBUG
        m_staticGuards[i] = std::move(m_staticGuards.back());
        m_staticGuards.pop_back();
#endif
    }

    for(Entity entity: entities) {
        if(entity >= MAX_ENTITIES) {
            continue;
        }

        if(m_registrationStates[entity] == RegistrationState::None) {
            registerEntity(entity, componentsArray);
        }
    }
}

void PhysicsSystem2D::unregisterEntity(Entity e){
    if(e >= MAX_ENTITIES) {
        return;
    }

    const RegistrationState state = m_registrationStates[e];

    if(state == RegistrationState::Dynamic) {
        auto iterator = std::find(
            m_dynamicEntities.begin(),
            m_dynamicEntities.end(),
            e
        );

        //Movemos el valor del final de la lista (es puntero) al puntero que tiene iterator, borrando el dato eliminado. Luego eliminamos ese
        //último lugar, que ahora esta inútil.
        if(iterator != m_dynamicEntities.end()) {
            *iterator = m_dynamicEntities.back();
            m_dynamicEntities.pop_back();
        }
    }

    if(state == RegistrationState::Static) {
        auto iterator = std::find_if(
            m_staticProxies.begin(),
            m_staticProxies.end(),
            [e](const ilen::physics::ColliderProxy2D& proxy) {
                return proxy.entity == e;
            }
        );

        if(iterator != m_staticProxies.end()) {
            const size_t index = static_cast<size_t>(
                std::distance(
                    m_staticProxies.begin(), 
                    iterator
                )
            );

            m_staticProxies[index] = std::move(m_staticProxies.back());
            m_staticProxies.pop_back();

#if ILEN_DEBUG
            m_staticGuards[index] = std::move(m_staticGuards.back());
            m_staticGuards.pop_back();
#endif
        }
    }

    m_registrationStates[e] = RegistrationState::None;
}

std::optional<ilen::physics::ColliderProxy2D> PhysicsSystem2D::buildProxy(
    Entity entity, ArrayOfComponentsUsed* componentsArray
) {
    auto* transform = componentsArray->transformComponentArray->getData(entity);
    auto* rb = componentsArray->rigidbodyComponentArray->getData(entity);
    auto* metadata = componentsArray->entityMetadataArray->getData(entity);

    if(!transform || !rb || !metadata) {
        return std::nullopt;
    }

    if(auto* box = componentsArray->boxColliderArray->getData(entity)) {
        return ilen::physics::ColliderProxyBuilder2D::FromBox(
            entity, 
            *transform ,
            *rb,
            *box,
            *metadata
        );
    }

    if(auto* polygon = componentsArray->convexPolygonCollider->getData(entity)) {
        return ilen::physics::ColliderProxyBuilder2D::FromPolygon(
            entity,
            *transform,
            *rb,
            *polygon,
            *metadata
        );
    }

    return std::nullopt;
}

void PhysicsSystem2D::registerPrevPosition(ArrayOfComponentsUsed* componentsArray){
    for(Entity e: m_dynamicEntities){
        auto* transform = componentsArray->transformComponentArray->getData(e);
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
    
        // Guardamos la memoria del frame anterior
        rb->prevPosition = vec2(transform->position.x, transform->position.y);
    }
}

void PhysicsSystem2D::clearCollisionData(ArrayOfComponentsUsed* componentsArray){
    for(Entity e: m_dynamicEntities){
        if(auto* col = componentsArray->collisionComponentArray->getData(e)){
            col->contacts.clear();
        }
    }
    for(const auto& proxy : m_staticProxies){
        if(auto* col = componentsArray->collisionComponentArray->getData(proxy.entity)){
            col->contacts.clear();
        }
    }
}

void PhysicsSystem2D::rebuildDynamicProxies(float dt, ArrayOfComponentsUsed* componentArray) {
    m_dynamicProxies.clear();
    m_dynamicProxies.reserve(m_dynamicEntities.size());

    for(Entity entity : m_dynamicEntities) {
        auto proxy = buildProxy(entity, componentArray);
        if(!proxy.has_value()) {
            continue;
        }

        auto* rb = componentArray->rigidbodyComponentArray->getData(entity);
        if(!rb) continue;

        translateProxy(*proxy, rb->velocity * dt);

        m_dynamicProxies.push_back(std::move(*proxy));
    }
}

void PhysicsSystem2D::translateProxy(
    ilen::physics::ColliderProxy2D& proxy, const vec2& translation
) {
    for(vec2& vertex : proxy.worldVertices) {
        vertex += translation;
    }

    proxy.bounds.center += translation;
}

void PhysicsSystem2D::applyForces(float dt, ArrayOfComponentsUsed* componentsArray){
    for(Entity e : m_dynamicEntities){
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
        auto* order = componentsArray->physicsOrderArray->getData(e);

        if(rb && order && order->hasOrders){
            //Calculamos la aceleracion: F = m * a => a = F / m
            vec2 acceleration = static_cast<vec2>(order->accumulatedForce) / rb->mass;
            rb->velocity += acceleration * dt;

            //Añadimos los impulsos. Estos no dependen de dt.
            rb->velocity += static_cast<vec2>(order->accumulatedImpulse) / rb->mass;

            order->accumulatedForce = vec3(0);
            order->accumulatedImpulse = vec3(0);
            order->hasOrders = false;
        }
    }
}

void PhysicsSystem2D::applyGravity(float dt, ArrayOfComponentsUsed* componentsArray){
    for(Entity e: m_dynamicEntities) {
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
        if(!rb || !rb->usesGravity /*|| rb->isInGround*/) continue; //Es increible que desactivar el calculo de la gravedad si se toca el cielo hace colapsar todo, XD.

        rb->velocity.y += ilen::core::options::PhysicsOptions::gravity * rb->gravityScale * dt;

        //Clamp a la velocidad máxima vertical del motor (tanto si va para arriba como abajo)
        if(std::abs(rb->velocity.y) > rb->terminalVelocity) {
            rb->velocity.y = (rb->velocity.y > 0) ? rb->terminalVelocity : -rb->terminalVelocity;
        }
    }//Entidades estáticas por concepto no pueden ser afectadas por la gravedad
}

void PhysicsSystem2D::integrate(float dt, ArrayOfComponentsUsed* componentsArray){
    auto* orderArray = p_coordinator->getComponentArray<components::TransformOrder>();

    for(Entity e : m_dynamicEntities) {
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
        if(!rb) continue;

        //ilen::engine::api::TransformAPI::Translate(e, static_cast<vec3>(rb->velocity * dt));
        TransformOrder order{};
        if(m_accumulatedTransformOrder.contains(e)) {
            order = m_accumulatedTransformOrder[e];
        }
        order.translation += static_cast<vec3>(rb->velocity * dt);

        m_accumulatedTransformOrder[e] = order;
        rb->isInGround = false;
    }
}

void PhysicsSystem2D::detectAndResolve(ArrayOfComponentsUsed* componentsArray) {
    size_t dynamicCount = m_dynamicProxies.size();
    size_t staticCount = m_staticProxies.size();  

   for(size_t i = 0; i < dynamicCount; i++) {
        auto& proxyA = m_dynamicProxies[i];
        for(size_t j = i + 1; j < dynamicCount; j++) {
            auto& proxyB = m_dynamicProxies[j];
            const auto layerA = static_cast<ilen::enumerations::ILEN_LAYER_SYSTEM>(proxyA.layerMask);
            const auto layerB = static_cast<ilen::enumerations::ILEN_LAYER_SYSTEM>(proxyB.layerMask);

            if(!ilen::physics::CollisionMatrix::CanCollide(
                layerA, layerB
            )) {
                continue;
            }

            auto manifold = ilen::physics::NarrowPhase2D::Test(proxyA, proxyB);
            if(manifold.has_value()) {
                resolveCollision(proxyA, proxyB, *manifold, componentsArray);
            }
        }

        for(size_t j = 0; j < staticCount; j++) {
            auto& proxyStatic = m_staticProxies[j];

            const auto dynamicLayer = static_cast<ilen::enumerations::ILEN_LAYER_SYSTEM>(proxyA.layerMask);
            const auto staticLayer = static_cast<ilen::enumerations::ILEN_LAYER_SYSTEM>(proxyStatic.layerMask);

            if(!ilen::physics::CollisionMatrix::CanCollide(dynamicLayer, staticLayer)) {
                continue;
            }

            auto manifold = ilen::physics::NarrowPhase2D::Test(proxyA, proxyStatic);

            if(manifold.has_value()) {
                resolveCollision(
                    proxyA, proxyStatic, *manifold, componentsArray
                );
            }
        }
    }
}

/*ilen::maths::aabb::AABB2D PhysicsSystem2D::buildAABB(Entity e, ArrayOfComponentsUsed* componentsArray) const {
    auto* transform = componentsArray->transformComponentArray->getData(e);
    auto* collider = componentsArray->boxColliderArray->getData(e);

    if(!transform || !collider) {
        return maths::aabb::AABB2D::fromCenterHalf(vec2(0), vec2(0));
    }

    vec2 post(transform->position.x, transform->position.y);
    vec2 center = post + collider->offset;

    vec2 scaledHalfExtents;
    scaledHalfExtents.x = collider->halfExtents.x * transform->scale.x;
    scaledHalfExtents.y = collider->halfExtents.y * transform->scale.y;

    return maths::aabb::AABB2D::fromCenterHalf(center, scaledHalfExtents);
}*/

void PhysicsSystem2D::resolveCollision(
    ilen::physics::ColliderProxy2D& proxyA,
    ilen::physics::ColliderProxy2D& proxyB,
    const physics::ContactManifold2D& manifold,
    ArrayOfComponentsUsed* componentsArray
){
    writeContacts(manifold, componentsArray);

    if(manifold.isTrigger) {
        return;
    }

    auto* rigidbodyA = componentsArray->rigidbodyComponentArray->getData(proxyA.entity);
    auto* rigidbodyB = componentsArray->rigidbodyComponentArray->getData(manifold.entityB);

    const bool dynamicA = rigidbodyA && !rigidbodyA->isStatic;
    const bool dynamicB = rigidbodyB && !rigidbodyB->isStatic;

    if(dynamicA && !dynamicB) {
        const vec2 separation = manifold.normal * manifold.penetration;
        m_accumulatedTransformOrder[proxyA.entity].translation += static_cast<vec3>(separation);
        translateProxy(proxyA, separation);

        const float normalVelocity = ilen::maths::vector::dot(rigidbodyA->velocity, manifold.normal);
        if(normalVelocity < 0.0f) {
            rigidbodyA->velocity -= manifold.normal * normalVelocity;
        }

        if(manifold.normal.y > 0.5f) {
            rigidbodyA->isInGround = true;
        }

        return;
    }

    if(dynamicA && dynamicB) {
        const vec2 separationA = manifold.normal * (manifold.penetration * 0.5f);
        const vec2 separationB = separationA * -1.0f;

        m_accumulatedTransformOrder[proxyA.entity].translation += static_cast<vec3>(separationA);
        m_accumulatedTransformOrder[proxyB.entity].translation += static_cast<vec3>(separationB);

        translateProxy(proxyA, separationA);
        translateProxy(proxyB, separationB);

        const float velocityA = ilen::maths::vector::dot(rigidbodyA->velocity, manifold.normal);
        if(velocityA < 0.0f) {
            rigidbodyA->velocity -= manifold.normal * velocityA;
        }

        const vec2 normalB = manifold.normal * -1.0f;
        const float velocityB = ilen::maths::vector::dot(rigidbodyB->velocity, normalB);

        if(velocityB < 0.0f) {
            rigidbodyB->velocity -= normalB * velocityB;
        }

        if(manifold.normal.y > 0.5f) {
            rigidbodyA->isInGround = true;
        }

        if (normalB.y > 0.5f) {
            rigidbodyB->isInGround = true;
        }
    }
}

void PhysicsSystem2D::writeContacts(const physics::ContactManifold2D& manifold, ArrayOfComponentsUsed* componentsArray){
    if(auto* collisionA = componentsArray->collisionComponentArray->getData(manifold.entityA)) {
        collisionA->contacts.push_back(manifold);
    }

    if(auto* collisionB = componentsArray->collisionComponentArray->getData(manifold.entityB)) {
        physics::ContactManifold2D mirrored = manifold;
        mirrored.entityA = manifold.entityB;
        mirrored.entityB = manifold.entityA;
        mirrored.normal = manifold.normal * -1.0f;

        collisionB->contacts.push_back(std::move(mirrored));
    }
}

#if ILEN_DEBUG
void PhysicsSystem2D::checkStaticGuard(ArrayOfComponentsUsed* componentsArray) {
    for(StaticGuard& guard : m_staticGuards) {
        guard.frameCounter--;
        if(guard.frameCounter > 0) {
            continue;
        }

        guard.frameCounter = guard.checkInterval;
        auto* transform = componentsArray->transformComponentArray->getData(guard.entity);
        auto* rigidbody = componentsArray->rigidbodyComponentArray->getData(guard.entity);

        if(!transform || !rigidbody) {
            continue;
        }

        if(
            transform->position != guard.originalPosition ||
            transform->rotation != guard.originalRotation ||
            transform->scale != guard.originalScale
        ) {
            ILEN_ERROR_P("PHYSICS_2D", "La entidad estática %u modifico su Transform", guard.entity);
        }

        if(!rigidbody->isStatic) {
            ILEN_ERROR_P("PHYSICS_2D", "La entidad %u fue registrada como estática y luego fue cambiado ese estado", guard.entity);
        }
    }
}
#endif