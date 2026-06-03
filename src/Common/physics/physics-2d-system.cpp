#include <ilen/physics/physics2d_system.h>
#include <ilen/headers/ecs.h>
#include <ilen/headers/ecs_components.h>
//#include <ilen_public/API/transform_api.h>
#include <ilen_public/API/physics2d_api.h>

using namespace ilen::ecs::systems;
using namespace ilen::ecs::components;

void PhysicsSystem2D::Init(const ilen::EngineContext* engineContext){
    p_coordinator = engineContext->p_coordinator.get();
    p_jobSystem = engineContext->p_jobSystem.get();
    p_commandDispatcher = engineContext->p_commandDispatcher.get();

    m_staticEntities.reserve(MAX_ENTITIES);
    m_dynamicEntities.reserve(MAX_ENTITIES);
    
#if ILEN_DEBUG
    m_staticGuards.reserve(MAX_ENTITIES);
#endif

    p_coordinator->registerComponent<physics::Rigidbody2D>();
    p_coordinator->registerComponent<physics::BoxCollider2D>();
    p_coordinator->registerComponent<physics::CollisionComponent2D>();

    ilen::engine::api::PhysicsAPI::Init(engineContext);
}


//TODO, hacer este sistema multihilo.
void PhysicsSystem2D::Update() {
    ilen::time::ScopedTimer timer("Physics 2D update");
   
    ArrayOfComponentsUsed componentArray{};
    
    componentArray.rigidbodyComponentArray = p_coordinator->getComponentArray<physics::Rigidbody2D>();
    componentArray.collisionComponentArray = p_coordinator->getComponentArray<physics::CollisionComponent2D>();
    componentArray.boxColliderArray = p_coordinator->getComponentArray<physics::BoxCollider2D>();
    componentArray.transformComponentArray = p_coordinator->getComponentArray<TransformComponent>();
    componentArray.physicsOrderArray = p_coordinator->getComponentArray<physics::PhysicsOrder>();
    componentArray.transformOrderArray = p_coordinator->getComponentArray<TransformOrder>();

    m_accumulatedTransformOrder.clear();


    syncEntities(&componentArray);

    //if(m_dynamicEntities.size() + m_staticEntities.size() != entities.size())
    if(m_dynamicEntities.empty() && m_staticEntities.empty()) return;

    constexpr float dt = 1.0f / ilen::core::options::PhysicsOptions::PhysicsUpdateTime;

    clearCollisionData(&componentArray);
    registerPrevPosition(&componentArray);
    applyForces(dt, &componentArray);
    applyGravity(dt, &componentArray);
    integrate(dt, &componentArray);
    detectAndResolve(dt, &componentArray);

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
    auto* collider = componentsArray->boxColliderArray->getData(e);

    if(!rb || !collider) return;

    //Método de seguridad para forzar el collider si este no existe.
    if (collider->halfExtents.x == 0.0f && collider->halfExtents.y == 0.0f) {
        ilen::engine::api::PhysicsAPI::GenerateBoxColliderSize(e, collider);
    }

    if(rb->isStatic) {
        m_staticEntities.push_back(e);
        m_staticAABBs.push_back(buildAABB(e, componentsArray));
#if ILEN_DEBUG
        auto* transform = p_coordinator->getComponent<TransformComponent>(e);
        StaticGuard guard{e, static_cast<vec2>(transform->position), 0, 60};
        m_staticGuards.push_back(guard);
#endif
    }else{
        m_dynamicEntities.push_back(e);
    }
}

void PhysicsSystem2D::syncEntities(ArrayOfComponentsUsed* componentsArray) {
    //Primero borramos las entidades que se hallan eliminado
    for(int i = (int)m_dynamicEntities.size() - 1; i >= 0; i--){
        if(entities.find(m_dynamicEntities[i]) == entities.end()){
            m_dynamicEntities[i] = m_dynamicEntities.back();
            m_dynamicEntities.pop_back();
        }
    }

    for(int i = (int)m_staticEntities.size() - 1; i >= 0; i--){
        if(entities.find(m_staticEntities[i]) == entities.end()){
            m_staticEntities[i] = m_staticEntities.back();
            m_staticAABBs[i] = m_staticAABBs.back();
            m_staticEntities.pop_back();
            m_staticAABBs.pop_back();

#if ILEN_DEBUG
            m_staticGuards[i] = m_staticGuards.back();
            m_staticGuards.pop_back();
#endif
        }
    }

    //Ahora, buscamos los nuevos, y los añadimos
    size_t totalInternos = m_dynamicEntities.size() + m_staticEntities.size();
    if(entities.size() > totalInternos){
        for(Entity e: entities) {
            bool inDynamic = std::find(m_dynamicEntities.begin(), m_dynamicEntities.end(), e) != m_dynamicEntities.end();
            bool inStatic = std::find(m_staticEntities.begin(), m_staticEntities.end(), e) != m_staticEntities.end();

            if(!inDynamic && !inStatic){
                registerEntity(e, componentsArray);
            }
        }
    }
}

void PhysicsSystem2D::unregisterEntity(Entity e){
    //Borramos los dinámicos
    auto itDyn = std::find(m_dynamicEntities.begin(), m_dynamicEntities.end(), e);
    if(itDyn != m_dynamicEntities.end()) {
        *itDyn = m_dynamicEntities.back();
        m_dynamicEntities.pop_back();
        return;
    }

    auto itStat = std::find(m_staticEntities.begin(), m_staticEntities.end(), e);
    if(itStat != m_staticEntities.end()){
        size_t index = std::distance(m_staticEntities.begin(), itStat);

        m_staticEntities[index] = m_staticEntities.back();
        m_staticAABBs[index] = m_staticAABBs.back();

        m_staticEntities.pop_back();
        m_staticAABBs.pop_back();
#if ILEN_DEBUG
        m_staticGuards[index] = m_staticGuards.back();
        m_staticGuards.pop_back();
#endif
    }
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
    for(Entity e: m_staticEntities){
        if(auto* col = componentsArray->collisionComponentArray->getData(e)){
            col->contacts.clear();
        }
    }
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

        rb->velocity.y += ilen::core::options::PhysicsOptions::gravity - rb->gravityScale * dt;

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

void PhysicsSystem2D::detectAndResolve(float dt, ArrayOfComponentsUsed* componentsArray) {
    size_t dynamicCount = m_dynamicEntities.size();
    size_t staticCount = m_staticEntities.size();  

    std::vector<AABB2D> dynamicsAABBs(dynamicCount);
    for(size_t i = 0; i < dynamicCount; i++){
        Entity e = m_dynamicEntities[i];
        dynamicsAABBs[i] = buildAABB(e, componentsArray);

        //ajustamos la AABB para el movimiento de este frame, realizado durante integrate.
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
        if(rb) {
            vec2 predictedMovement = rb->velocity * dt;
            dynamicsAABBs[i].center += predictedMovement;
        }
    }
    //

    //Bucle para detectar colisiones (a futuro hay que descartar rapidamente antes de hacer este bucle usando un VBB Volume Bounding Box, o algo así se llama)
    for(size_t i = 0; i < dynamicCount; i++){
        Entity a = m_dynamicEntities[i];
        maths::aabb::AABB2D aabbA = dynamicsAABBs[i];
        auto* rbA = componentsArray->rigidbodyComponentArray->getData(a);

        //Colisiones Dynamic vs Dynamic
        for(size_t j = i + 1; j < dynamicCount; j++){
            Entity b = m_dynamicEntities[j];
            maths::aabb::AABB2D aabbB = dynamicsAABBs[j];
            auto* rbB = componentsArray->rigidbodyComponentArray->getData(b);


            auto result = maths::aabb::intersectAABB2D<CollisionResolutionType::DynamicVsDynamic>(aabbA, aabbB, rbA->prevPosition, rbB->prevPosition);

            if(result.hit) {
                resolveCollision(a, b, result, componentsArray);

                //Actulizamos los aabb por esa colision.
                vec2 correctionA = result.normal * (result.depth * 0.5f);
                vec2 correctionB = correctionA * -1.0f;

                dynamicsAABBs[i].center +=correctionA;
                dynamicsAABBs[j].center += correctionB;
            }
        }

        //Colisiones dynamic vs Static
        for(size_t j = 0; j < staticCount; j++){
            Entity s = m_staticEntities[j];
            const maths::aabb::AABB2D& aabbStatic = m_staticAABBs[j];

            auto result = maths::aabb::intersectAABB2D<CollisionResolutionType::DynamicVsStatic>(aabbA, aabbStatic, rbA->prevPosition);
            if(result.hit){
                resolveCollision(a, s, result, componentsArray);

                //Actulizamos los aabb por esa colision.
                vec2 correctionA = result.normal * (result.depth * 0.5f);
                dynamicsAABBs[i].center +=correctionA;
            }
        }
    }
}

ilen::maths::aabb::AABB2D PhysicsSystem2D::buildAABB(Entity e, ArrayOfComponentsUsed* componentsArray) const {
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
}

void PhysicsSystem2D::resolveCollision(Entity a, Entity b, const ilen::maths::aabb::IntersectResult2D& result, ArrayOfComponentsUsed* componentsArray){
    auto* colA = componentsArray->boxColliderArray->getData(a);
    auto* colB = componentsArray->boxColliderArray->getData(b);


    bool isTriggerCollision = colA->isTrigger || colB->isTrigger;
    writeContact(a, b, result, isTriggerCollision, componentsArray);

    maths::aabb::IntersectResult2D resultB = result;
    resultB.normal.x = -result.normal.x;
    resultB.normal.y = -result.normal.y;
    writeContact(b, a, resultB, isTriggerCollision, componentsArray);

    if(isTriggerCollision) return;

    //Corrección de la transformación
    auto* rbA = componentsArray->rigidbodyComponentArray->getData(a);
    auto* rbB = componentsArray->rigidbodyComponentArray->getData(b);
    
    bool aIsDynamic = rbA && !rbA->isStatic;
    bool bIsDynamic = rbB && !rbB->isStatic;

    if(aIsDynamic && !bIsDynamic){
        //Elemento dinamico choca con objeto estático. Eso significa que el dinamico penetra el 100%
        vec3 separation(result.normal * result.depth);
        //ilen::engine::api::TransformAPI::Translate(a, separation);
        TransformOrder order{};
        if(m_accumulatedTransformOrder.contains(a)) {
            order = m_accumulatedTransformOrder[a];
        }
        order.translation += separation;
        
        m_accumulatedTransformOrder[a] = order;

        if(std::abs(result.normal.y) > 0.0f){
            rbA->velocity.y = 0.0f;
            if(result.normal.y > 0.0f) rbA->isInGround = true;
        }

        if(std::abs(result.normal.x) > 0.0f){
            rbA->velocity.x = 0.0f;
        }
    } else if( aIsDynamic && bIsDynamic){
        //Dinamico vs Dinamico. Digamos que se prenentan un 50% cada uno.
        vec3 separationA(result.normal * (result.depth * 0.5f));
        vec3 separationB(separationA * -1.0f);

        //ilen::engine::api::TransformAPI::Translate(a, separationA);
        //ilen::engine::api::TransformAPI::Translate(b, separationB);
        TransformOrder order{};
        if(m_accumulatedTransformOrder.contains(a)) {
            order = m_accumulatedTransformOrder[a];
        }
        order.translation += separationA;
        
        m_accumulatedTransformOrder[a] = order;

        order = {};
        if(m_accumulatedTransformOrder.contains(b)) {
            order = m_accumulatedTransformOrder[b];
        }
        order.translation += separationB;
        
        m_accumulatedTransformOrder[b] = order;

        if(std::abs(result.normal.y) > 0.0f){
            rbA->velocity.y = 0.0f;
            rbB->velocity.y = 0.0f;
        }

        if(std::abs(result.normal.x) > 0.0f){
            rbA->velocity.x = 0.0f;
            rbB->velocity.x = 0.0f;
        }
    }
}

void PhysicsSystem2D::writeContact(Entity receiver, Entity other, const ilen::maths::aabb::IntersectResult2D& result, bool wasTrigger, ArrayOfComponentsUsed* componentsArray){
    if(auto* colComp = componentsArray->collisionComponentArray->getData(receiver)) {
        physics::CollisionContact2D contact;
        contact.other = other;
        contact.normal = result.normal;
        contact.depth = result.depth;
        contact.wasTrigger = wasTrigger;
        colComp->contacts.push_back(contact);
    }
}

#if ILEN_DEBUG
void PhysicsSystem2D::checkStaticGuard(ArrayOfComponentsUsed* componentsArray) {
    for (auto& guard : m_staticGuards) {
                guard.frameCounter--;

        if (guard.frameCounter <= 0) {
            if (auto* transform = componentsArray->transformComponentArray->getData(guard.entity)) {
                if (transform->position.x != guard.originalPosition.x || 
                    transform->position.y != guard.originalPosition.y) {
                    
                    std::cerr << "[ILEN_PHYSICS_ERROR] Entidad estatica (ID: " << guard.entity 
                            << ") ha sido movida! Pos original: (" << guard.originalPosition.x << ", " 
                            << guard.originalPosition.y << ") Pos actual: (" 
                            << transform->position.x << ", " << transform->position.y << ").\n";
                            
                    guard.originalPosition = maths::vector::vec2(transform->position.x, transform->position.y);
                }
            }
            guard.frameCounter = guard.checkInterval;
        }
            }
}
#endif