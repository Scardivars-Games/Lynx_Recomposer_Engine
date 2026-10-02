#include <algorithm>
#include <ilen/physics/physics_2d_system.h>
#include <ilen/headers/ecs.h>
#include <ilen/headers/ecs_components.h>
#include <ilen_public/API/physics_2d_api.h>
#include <iterator>
#include <optional>
#include <utility>
#include "ilen/maths/AABB.h"
#include "ilen/physics/narrow_phase_2d.h"


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
    rebuildDynamicProxies(dt, &componentArray);
    integrate(dt, &componentArray);
    detectAndResolve(dt, &componentArray);

#if ILEN_DEBUG
    reportCollisionDebugStats();
#endif

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
        m_staticBroadPhaseDirty = true;
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
        m_staticBroadPhaseDirty = true;
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
            m_staticBroadPhaseDirty = true;

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

void PhysicsSystem2D::rebuildDynamicProxies(
    float dt,
    ArrayOfComponentsUsed* componentArray
) {
    m_dynamicProxies.clear();
    m_dynamicProxies.reserve(m_dynamicEntities.size());

    for(Entity entity : m_dynamicEntities) {
        auto proxy = buildProxy(entity, componentArray);
        if(!proxy.has_value()) {
            continue;
        }

        auto* rigidbody =
            componentArray->rigidbodyComponentArray->getData(entity);
        auto* transform =
            componentArray->transformComponentArray->getData(entity);

        if(!rigidbody || !transform) {
            continue;
        }

        // Teleport cambia el origen del proxy sin barrer el trayecto. El SAT
        // discreto del solver resolverá cualquier contacto en el destino.
        if(m_hasTeleport[entity]) {
            const vec2 teleportDelta =
                m_teleportTargets[entity] - static_cast<vec2>(transform->position);

            translateProxy(*proxy, teleportDelta);
            m_accumulatedTransformOrder[entity].translation +=
                static_cast<vec3>(teleportDelta);
        }

        proxy->commandedVelocity = dt > 0.0f
            ? m_requestedTranslations[entity] / dt
            : vec2(0.0f);

        proxy->motionVelocity =
            rigidbody->velocity + proxy->commandedVelocity;

        // El proxy comienza en la posición actual o teletransportada. La
        // integración continua lo avanzará hasta el primer impacto.
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

AABB2D PhysicsSystem2D::calculateSweptBounds(
    const ilen::physics::ColliderProxy2D& proxy,
    const vec2& translation
) {
    AABB2D endBounds = proxy.bounds;
    endBounds.center += translation;

    return AABB2D::fromMinMax(
        ilen::maths::vector::min(proxy.bounds.min(), endBounds.min()),
        ilen::maths::vector::max(proxy.bounds.max(), endBounds.max())
    );
}

void PhysicsSystem2D::applyForces(float dt, ArrayOfComponentsUsed* componentsArray){
    for(Entity e : m_dynamicEntities){
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
        auto* order = componentsArray->physicsOrderArray->getData(e);

        m_requestedTranslations[e] = vec2(0.0f);
        m_teleportTargets[e] = vec2(0.0f);
        m_hasTeleport[e] = false;

        if(rb && order && order->hasOrders){
            // Las velocidades pedidas por la API deben existir antes de
            // calcular fuerzas y gravedad para que el CCD barra exactamente
            // el movimiento que ejecutará este fixed update.
            if(order->overrideVelocityX) {
                rb->velocity.x = order->velocityOverride.x;
            }

            if(order->overrideVelocityY) {
                rb->velocity.y = order->velocityOverride.y;
            }

            auto* transform =
                componentsArray->transformComponentArray->getData(e);

            if(order->hasTeleport) {
                m_teleportTargets[e] = order->targetPosition;
                m_hasTeleport[e] = true;
            } else if(order->hasMovePosition && transform) {
                m_requestedTranslations[e] =
                    order->targetPosition - static_cast<vec2>(transform->position);
            }

            m_requestedTranslations[e] += order->accumulatedTranslation;

            if(!rb->isKinematic) {
                //Calculamos la aceleracion: F = m * a => a = F / m
                vec2 acceleration = static_cast<vec2>(order->accumulatedForce) / rb->mass;
                rb->velocity += acceleration * dt;

                //Añadimos los impulsos. Estos no dependen de dt.
                rb->velocity += static_cast<vec2>(order->accumulatedImpulse) / rb->mass;
            }

            order->accumulatedForce = vec3(0);
            order->accumulatedImpulse = vec3(0);
            order->velocityOverride = vec2(0.0f);
            order->overrideVelocityX = false;
            order->overrideVelocityY = false;
            order->accumulatedTranslation = vec2(0.0f);
            order->targetPosition = vec2(0.0f);
            order->hasMovePosition = false;
            order->hasTeleport = false;
            order->hasOrders = false;
        }
    }
}

void PhysicsSystem2D::applyGravity(float dt, ArrayOfComponentsUsed* componentsArray){
    for(Entity e: m_dynamicEntities) {
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
        if(!rb || rb->isKinematic || !rb->usesGravity /*|| rb->isInGround*/) continue; //Es increible que desactivar el calculo de la gravedad si se toca el cielo hace colapsar todo, XD.

        rb->velocity.y += ilen::core::options::PhysicsOptions::gravity * rb->gravityScale * dt;

        //Clamp a la velocidad máxima vertical del motor (tanto si va para arriba como abajo)
        if(std::abs(rb->velocity.y) > rb->terminalVelocity) {
            rb->velocity.y = (rb->velocity.y > 0) ? rb->terminalVelocity : -rb->terminalVelocity;
        }
    }//Entidades estáticas por concepto no pueden ser afectadas por la gravedad
}

void PhysicsSystem2D::integrate(float dt, ArrayOfComponentsUsed* componentsArray){
    // La integración ya no escribe inmediatamente velocity * dt en Transform.
    // Primero prepara el volumen barrido; detectAndResolve decidirá cuánto del
    // movimiento puede consumirse antes de cada impacto.
    for(Entity e : m_dynamicEntities) {
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
        if(!rb) continue;
        rb->isInGround = false;
    }

    prepareSweeps(dt, componentsArray);
}

void PhysicsSystem2D::prepareSweeps(
    float remainingTime,
    ArrayOfComponentsUsed* componentsArray
) {
    for(auto& proxy : m_dynamicProxies) {
        auto* rigidbody =
            componentsArray->rigidbodyComponentArray->getData(proxy.entity);

        if(!rigidbody) {
            proxy.stepTranslation = vec2{0.0f};
            proxy.sweptBounds = proxy.bounds;
            continue;
        }

        proxy.stepTranslation = proxy.motionVelocity * remainingTime;
        proxy.sweptBounds = calculateSweptBounds(
            proxy,
            proxy.stepTranslation
        );
    }
}

void PhysicsSystem2D::advanceDynamicProxies(float timeFraction) {
    for(auto& proxy : m_dynamicProxies) {
        const vec2 translation = proxy.stepTranslation * timeFraction;

        if(translation.x == 0.0f && translation.y == 0.0f) {
            proxy.sweptBounds = proxy.bounds;
            continue;
        }

        m_accumulatedTransformOrder[proxy.entity].translation +=
            static_cast<vec3>(translation);

        translateProxy(proxy, translation);
        proxy.sweptBounds = proxy.bounds;
    }
}

void PhysicsSystem2D::detectAndResolve(
    float dt,
    ArrayOfComponentsUsed* componentsArray
) {
    // El límite protege frente a ciclos de TOI=0. No representa pasadas
    // obligatorias: sin contactos el bucle termina en su primera iteración.
    constexpr uint32_t maximumCollisionEvents = 8;
    constexpr float minimumRemainingTime = 0.000001f;

#if ILEN_DEBUG
    m_collisionDebugStats.reset();
#endif

    if(m_staticBroadPhaseDirty) {
        m_broadPhase.RebuildStaticGrid(m_staticProxies);
        m_staticBroadPhaseDirty = false;
    }

    float remainingTime = dt;

    for(
        uint32_t eventIndex = 0;
        eventIndex < maximumCollisionEvents && remainingTime > minimumRemainingTime;
        ++eventIndex
    ) {
        prepareSweeps(remainingTime, componentsArray);
        const auto& candidatePairs =
            m_broadPhase.BuildCandidatesPairs(m_dynamicProxies);

#if ILEN_DEBUG
        const size_t dynamicCount = m_dynamicProxies.size();
        const size_t staticCount = m_staticProxies.size();
        m_collisionDebugStats.collisionPasses++;
        m_collisionDebugStats.bruteForcePairs +=
            (dynamicCount * (dynamicCount > 0 ? dynamicCount - 1 : 0)) / 2 +
            dynamicCount * staticCount;
        m_collisionDebugStats.broadPhaseCandidates += candidatePairs.size();
#endif

        struct ContactEvent {
            ilen::physics::ColliderProxy2D* proxyA = nullptr;
            ilen::physics::ColliderProxy2D* proxyB = nullptr;
            physics::ContactManifold2D manifold{};
        };

        constexpr float simultaneousImpactEpsilon = 0.00001f;
        std::optional<float> earliestImpactTime;
        std::vector<ContactEvent> earliestImpacts;
        std::vector<ContactEvent> penetrations;
        earliestImpacts.reserve(candidatePairs.size());
        penetrations.reserve(candidatePairs.size());
        std::vector<physics::ContactManifold2D> triggerImpacts;
        triggerImpacts.reserve(candidatePairs.size());

        for(const auto& pair : candidatePairs) {
            if(!pair.proxyA || !pair.proxyB) {
                continue;
            }

            auto& proxyA = *pair.proxyA;
            auto& proxyB = *pair.proxyB;

#if ILEN_DEBUG
            if(proxyB.isStatic) {
                m_collisionDebugStats.dynamicVsStaticPairs++;
            } else {
                m_collisionDebugStats.dynamicVsDynamicPairs++;
            }
#endif

            const auto layerA =
                static_cast<ilen::enumerations::ILEN_LAYER_SYSTEM>(proxyA.layerMask);
            const auto layerB =
                static_cast<ilen::enumerations::ILEN_LAYER_SYSTEM>(proxyB.layerMask);

            if(!ilen::physics::CollisionMatrix::CanCollide(layerA, layerB)) {
#if ILEN_DEBUG
                m_collisionDebugStats.rejectedByLayer++;
#endif
                continue;
            }

#if ILEN_DEBUG
            m_collisionDebugStats.aabbTests++;
#endif

            const bool overlapsNow = ilen::maths::aabb::overlapAABB2D(
                proxyA.bounds,
                proxyB.bounds
            );

            if(overlapsNow) {
#if ILEN_DEBUG
                m_collisionDebugStats.satTests++;
#endif

                auto manifold = ilen::physics::NarrowPhase2D::Test(proxyA, proxyB);

                if(manifold.has_value()) {
#if ILEN_DEBUG
                    m_collisionDebugStats.collisionDetected++;
#endif

                    if(manifold->isTrigger) {
                        writeContacts(*manifold, componentsArray);
                    } else {
                        penetrations.push_back({
                            .proxyA = &proxyA,
                            .proxyB = &proxyB,
                            .manifold = std::move(*manifold)
                        });
                    }

                    continue;
                }
            }

            if(!ilen::maths::aabb::overlapAABB2D(proxyA.sweptBounds, proxyB.sweptBounds)) {
#if ILEN_DEBUG
                m_collisionDebugStats.rejectedByAABB++;
#endif
                continue;
            }

#if ILEN_DEBUG
            m_collisionDebugStats.satTests++;
#endif

            auto manifold = ilen::physics::NarrowPhase2D::Sweep(proxyA, proxyB);
            if(!manifold.has_value()) {
                continue;
            }

#if ILEN_DEBUG
            m_collisionDebugStats.collisionDetected++;
#endif

            // Los triggers informan del cruce, pero nunca limitan el movimiento.
            if(manifold->isTrigger) {
                triggerImpacts.push_back(std::move(*manifold));
                continue;
            }

            if(
                !earliestImpactTime.has_value() ||
                manifold->timeOfImpact <
                    *earliestImpactTime - simultaneousImpactEpsilon
            ) {
                earliestImpactTime = manifold->timeOfImpact;
                earliestImpacts.clear();
                earliestImpacts.push_back({
                    .proxyA = &proxyA,
                    .proxyB = &proxyB,
                    .manifold = std::move(*manifold)
                });
            } else if(
                std::abs(manifold->timeOfImpact - *earliestImpactTime) <=
                    simultaneousImpactEpsilon
            ) {
                earliestImpacts.push_back({
                    .proxyA = &proxyA,
                    .proxyB = &proxyB,
                    .manifold = std::move(*manifold)
                });
            }
        }

        // Todas las penetraciones actuales pertenecen al mismo instante. Si se
        // resolviera solamente la más profunda, una pila jugador-caja-suelo
        // consumiría el límite de eventos alternando entre sus dos contactos.
        if(!penetrations.empty()) {
            for(auto& contact : penetrations) {
                resolveCollision(
                    *contact.proxyA,
                    *contact.proxyB,
                    contact.manifold,
                    componentsArray
                );
            }
            continue;
        }

        const float allowedTimeFraction = earliestImpactTime.has_value()
            ? *earliestImpactTime
            : 1.0f;

        // Un trigger situado después de una pared no debe activarse si esa pared
        // detuvo el movimiento antes de alcanzarlo.
        for(const auto& triggerImpact : triggerImpacts) {
            if(triggerImpact.timeOfImpact <= allowedTimeFraction) {
                writeContacts(triggerImpact, componentsArray);
            }
        }

        if(!earliestImpactTime.has_value()) {
            // Nada bloquea el tiempo restante: todos los cuerpos llegan a su
            // posición prevista y termina la integración de este fixed update.
            advanceDynamicProxies(1.0f);
            remainingTime = 0.0f;
            break;
        }

        const float timeOfImpact = std::clamp(
            *earliestImpactTime,
            0.0f,
            1.0f
        );

        advanceDynamicProxies(timeOfImpact);
        const float remainingTimeAfterImpact =
            remainingTime * (1.0f - timeOfImpact);

        // Gauss-Seidel sobre la isla del mismo TOI. Repetir este conjunto
        // propaga en el mismo evento que la caja está bloqueada por la pared o
        // por el suelo, sin consumir ocho eventos a tiempo cero.
        constexpr uint32_t simultaneousSolverIterations = 16;
        constexpr float solvedVelocityEpsilon = 0.0001f;

        struct BlockedDirection {
            ilen::physics::ColliderProxy2D* proxy = nullptr;
            vec2 normal{0.0f};
        };

        std::vector<BlockedDirection> blockedDirections;
        blockedDirections.reserve(candidatePairs.size());

        for(
            uint32_t solverIteration = 0;
            solverIteration < simultaneousSolverIterations;
            ++solverIteration
        ) {
            for(auto& contact : earliestImpacts) {
                resolveCollision(
                    *contact.proxyA,
                    *contact.proxyB,
                    contact.manifold,
                    componentsArray
                );
            }

            // Una restricción inicialmente en reposo puede activarse al recibir
            // movimiento desde otra: jugador -> caja -> pared. Se reutilizan
            // las parejas del broadphase y solamente se repite el SAT continuo
            // para descubrir esos contactos a tiempo cero.
            prepareSweeps(remainingTimeAfterImpact, componentsArray);
            bool addedConstraint = false;

            for(const auto& pair : candidatePairs) {
                if(!pair.proxyA || !pair.proxyB) {
                    continue;
                }

                const auto layerA = static_cast<
                    ilen::enumerations::ILEN_LAYER_SYSTEM
                >(pair.proxyA->layerMask);

                const auto layerB = static_cast<
                    ilen::enumerations::ILEN_LAYER_SYSTEM
                >(pair.proxyB->layerMask);

                if(!ilen::physics::CollisionMatrix::CanCollide(layerA, layerB)) {
                    continue;
                }

                const bool alreadyInIsland = std::any_of(
                    earliestImpacts.begin(),
                    earliestImpacts.end(),
                    [&pair](const ContactEvent& contact) {
                        return
                            (contact.proxyA == pair.proxyA &&
                             contact.proxyB == pair.proxyB) ||
                            (contact.proxyA == pair.proxyB &&
                             contact.proxyB == pair.proxyA);
                    }
                );

                if(alreadyInIsland) {
                    continue;
                }

                if(!ilen::maths::aabb::overlapAABB2D(
                    pair.proxyA->sweptBounds,
                    pair.proxyB->sweptBounds
                )) {
                    continue;
                }

#if ILEN_DEBUG
                m_collisionDebugStats.satTests++;
#endif

                auto propagatedImpact = ilen::physics::NarrowPhase2D::Sweep(
                    *pair.proxyA,
                    *pair.proxyB
                );

                if(
                    !propagatedImpact.has_value() ||
                    propagatedImpact->isTrigger ||
                    propagatedImpact->timeOfImpact > simultaneousImpactEpsilon
                ) {
                    continue;
                }

#if ILEN_DEBUG
                m_collisionDebugStats.collisionDetected++;
#endif

                earliestImpacts.push_back({
                    .proxyA = pair.proxyA,
                    .proxyB = pair.proxyB,
                    .manifold = std::move(*propagatedImpact)
                });
                addedConstraint = true;
            }

            blockedDirections.clear();

            const auto isBlocked = [&blockedDirections](
                const ilen::physics::ColliderProxy2D* proxy,
                const vec2& normal
            ) {
                return std::any_of(
                    blockedDirections.begin(),
                    blockedDirections.end(),
                    [proxy, &normal](const BlockedDirection& blocked) {
                        return blocked.proxy == proxy &&
                            ilen::maths::vector::dot(
                                blocked.normal,
                                normal
                            ) > 0.95f;
                    }
                );
            };

            const auto addBlockedDirection = [
                &blockedDirections,
                &isBlocked
            ](
                ilen::physics::ColliderProxy2D* proxy,
                const vec2& normal
            ) {
                if(!proxy || proxy->isStatic || isBlocked(proxy, normal)) {
                    return false;
                }

                blockedDirections.push_back({
                    .proxy = proxy,
                    .normal = normal
                });
                return true;
            };

            // Las superficies estáticas son los anclajes de la isla. Su
            // restricción se transmite por los contactos paralelos para evitar
            // que razones de masa extremas necesiten cientos de iteraciones.
            for(const auto& contact : earliestImpacts) {
                if(!contact.proxyA->isStatic && contact.proxyB->isStatic) {
                    addBlockedDirection(
                        contact.proxyA,
                        contact.manifold.normal
                    );
                } else if(
                    contact.proxyA->isStatic &&
                    !contact.proxyB->isStatic
                ) {
                    addBlockedDirection(
                        contact.proxyB,
                        contact.manifold.normal * -1.0f
                    );
                }
            }

            for(
                size_t propagationPass = 0;
                propagationPass < earliestImpacts.size();
                ++propagationPass
            ) {
                bool propagated = false;

                for(const auto& contact : earliestImpacts) {
                    if(contact.proxyA->isStatic || contact.proxyB->isStatic) {
                        continue;
                    }

                    if(isBlocked(contact.proxyB, contact.manifold.normal)) {
                        propagated |= addBlockedDirection(
                            contact.proxyA,
                            contact.manifold.normal
                        );
                    }

                    const vec2 normalB = contact.manifold.normal * -1.0f;
                    if(isBlocked(contact.proxyA, normalB)) {
                        propagated |= addBlockedDirection(
                            contact.proxyB,
                            normalB
                        );
                    }
                }

                if(!propagated) {
                    break;
                }
            }

            for(const auto& blocked : blockedDirections) {
                const float normalMotion = ilen::maths::vector::dot(
                    blocked.proxy->motionVelocity,
                    blocked.normal
                );

                if(normalMotion < 0.0f) {
                    blocked.proxy->motionVelocity -=
                        blocked.normal * normalMotion;
                }

                auto* rigidbody =
                    componentsArray->rigidbodyComponentArray->getData(
                        blocked.proxy->entity
                    );

                if(!rigidbody) {
                    continue;
                }

                const float rigidbodyNormalVelocity =
                    ilen::maths::vector::dot(
                        rigidbody->velocity,
                        blocked.normal
                    );

                if(rigidbodyNormalVelocity < 0.0f) {
                    rigidbody->velocity -=
                        blocked.normal * rigidbodyNormalVelocity;
                }
            }

            // Una sola restricción suele quedar resuelta en la primera pasada.
            // Las iteraciones adicionales solamente son necesarias cuando un
            // contacto transmite movimiento a otro, como jugador-caja-pared.
            bool hasUnsolvedConstraint = addedConstraint;
            for(const auto& contact : earliestImpacts) {
                const float relativeNormalVelocity =
                    ilen::maths::vector::dot(
                        contact.proxyA->motionVelocity -
                            contact.proxyB->motionVelocity,
                        contact.manifold.normal
                    );

                if(relativeNormalVelocity < -solvedVelocityEpsilon) {
                    hasUnsolvedConstraint = true;
                    break;
                }
            }

            if(!hasUnsolvedConstraint) {
                break;
            }
        }

        remainingTime = remainingTimeAfterImpact;
    }
}

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

        const float normalMotion =
            ilen::maths::vector::dot(proxyA.motionVelocity, manifold.normal);

        if(normalMotion < 0.0f) {
            proxyA.motionVelocity -= manifold.normal * normalMotion;
        }

        const float rigidbodyNormalVelocity =
            ilen::maths::vector::dot(rigidbodyA->velocity, manifold.normal);

        if(rigidbodyNormalVelocity < 0.0f) {
            rigidbodyA->velocity -=
                manifold.normal * rigidbodyNormalVelocity;
        }

        if(manifold.normal.y > 0.5f) {
            rigidbodyA->isInGround = true;
        }

        return;
    }

    const bool kinematicA = dynamicA && rigidbodyA->isKinematic;
    const bool kinematicB = dynamicB && rigidbodyB->isKinematic;

    if(kinematicA && kinematicB) {
        const vec2 separationA =
            manifold.normal * (manifold.penetration * 0.5f);
        const vec2 separationB = separationA * -1.0f;

        m_accumulatedTransformOrder[proxyA.entity].translation +=
            static_cast<vec3>(separationA);
        m_accumulatedTransformOrder[proxyB.entity].translation +=
            static_cast<vec3>(separationB);
        translateProxy(proxyA, separationA);
        translateProxy(proxyB, separationB);

        const float relativeNormalVelocity = ilen::maths::vector::dot(
            proxyA.motionVelocity - proxyB.motionVelocity,
            manifold.normal
        );

        if(relativeNormalVelocity < 0.0f) {
            const vec2 correction =
                manifold.normal * (-relativeNormalVelocity * 0.5f);
            proxyA.motionVelocity += correction;
            proxyB.motionVelocity -= correction;
        }
        return;
    }

    // Un cinemático actúa como masa infinita frente a un dinámico: conserva
    // su movimiento solicitado y transmite al otro cuerpo la componente normal.
    if(dynamicA && dynamicB && (kinematicA != kinematicB)) {
        const float relativeNormalVelocity = ilen::maths::vector::dot(
            proxyA.motionVelocity - proxyB.motionVelocity,
            manifold.normal
        );

        if(kinematicA) {
            const vec2 separationB =
                manifold.normal * -manifold.penetration;

            m_accumulatedTransformOrder[proxyB.entity].translation +=
                static_cast<vec3>(separationB);
            translateProxy(proxyB, separationB);

            if(relativeNormalVelocity < 0.0f) {
                const vec2 transferredVelocity =
                    manifold.normal * relativeNormalVelocity;

                proxyB.motionVelocity += transferredVelocity;

                // Move/MovePosition son órdenes de este paso, no impulsos.
                // El cuerpo empujado se desplaza ahora, pero no conserva una
                // velocidad infinita cuando el cinemático deja de empujarlo.
                if(std::abs(ilen::maths::vector::dot(
                    proxyA.commandedVelocity,
                    manifold.normal
                )) <= 0.0001f) {
                    rigidbodyB->velocity += transferredVelocity;
                }
            }
        } else {
            const vec2 separationA =
                manifold.normal * manifold.penetration;

            m_accumulatedTransformOrder[proxyA.entity].translation +=
                static_cast<vec3>(separationA);
            translateProxy(proxyA, separationA);

            if(relativeNormalVelocity < 0.0f) {
                const vec2 transferredVelocity =
                    manifold.normal * -relativeNormalVelocity;

                proxyA.motionVelocity += transferredVelocity;

                if(std::abs(ilen::maths::vector::dot(
                    proxyB.commandedVelocity,
                    manifold.normal
                )) <= 0.0001f) {
                    rigidbodyA->velocity += transferredVelocity;
                }
            }
        }

        if(manifold.normal.y > 0.5f) rigidbodyA->isInGround = true;
        if(manifold.normal.y < -0.5f) rigidbodyB->isInGround = true;
        return;
    }

    if(
        dynamicA &&
        dynamicB &&
        ilen::core::options::PhysicsOptions::solveRealisticCollision
    ) {
        constexpr float minimumMass = 0.0001f;
        constexpr float movementEpsilon = 0.0001f;
        constexpr float momentumEpsilon = 0.0001f;
        constexpr float maximumPushRatio = 2.0f;

        const float massA = std::max(rigidbodyA->mass, minimumMass);
        const float massB = std::max(rigidbodyB->mass, minimumMass);

        const float approachA = std::max(
            0.0f,
            -ilen::maths::vector::dot(
                proxyA.motionVelocity,
                manifold.normal
            )
        );

        const float approachB = std::max(
            0.0f,
            ilen::maths::vector::dot(
                proxyB.motionVelocity,
                manifold.normal
            )
        );

        // Driver identifica qué cuerpo generó el acercamiento. La normal SAT
        // siempre expulsa A de B, por eso los signos de A y B son opuestos.
        enum class CollisionDriver {
            None,
            A,
            B,
            Mutual
        };

        CollisionDriver driver = CollisionDriver::None;

        if(
            approachA > movementEpsilon &&
            approachB <= movementEpsilon
        ) {
            driver = CollisionDriver::A;
        } else if(
            approachB > movementEpsilon &&
            approachA <= movementEpsilon
        ) {
            driver = CollisionDriver::B;
        } else if(
            approachA > movementEpsilon &&
            approachB > movementEpsilon
        ) {
            const float momentumA = massA * approachA;
            const float momentumB = massB * approachB;

            if(std::abs(momentumA - momentumB) <= momentumEpsilon) {
                driver = CollisionDriver::Mutual;
            } else {
                driver = momentumA > momentumB
                    ? CollisionDriver::A
                    : CollisionDriver::B;
            }
        }

        const bool aCanPushB =
            massB <= massA * maximumPushRatio + minimumMass;

        const bool bCanPushA =
            massA <= massB * maximumPushRatio + minimumMass;

        float movementWeightA = 0.0f;
        float movementWeightB = 0.0f;

        // Si el receptor supera el doble de la masa del impulsor, se comporta
        // como inamovible para este contacto. En los demás casos se usan masas
        // inversas: masas iguales producen exactamente un reparto 50/50.
        if(driver == CollisionDriver::A && !aCanPushB) {
            movementWeightA = 1.0f;
            movementWeightB = 0.0f;
        } else if(driver == CollisionDriver::B && !bCanPushA) {
            movementWeightA = 0.0f;
            movementWeightB = 1.0f;
        } else {
            const float inverseMassA = 1.0f / massA;
            const float inverseMassB = 1.0f / massB;
            const float totalInverseMass = inverseMassA + inverseMassB;

            movementWeightA = inverseMassA / totalInverseMass;
            movementWeightB = inverseMassB / totalInverseMass;
        }

        // El manifold sólo existe cuando hay solapamiento real. Restar slop
        // podía convertir contactos pequeños en una corrección cero; el mismo
        // contacto se elegía ocho veces y agotaba el paso sin mover en X.
        const float correctedPenetration = manifold.penetration;

        const vec2 separationA =
            manifold.normal *
            (correctedPenetration * movementWeightA);

        const vec2 separationB =
            manifold.normal *
            (-correctedPenetration * movementWeightB);

        m_accumulatedTransformOrder[proxyA.entity].translation +=
            static_cast<vec3>(separationA);

        m_accumulatedTransformOrder[proxyB.entity].translation +=
            static_cast<vec3>(separationB);

        translateProxy(proxyA, separationA);
        translateProxy(proxyB, separationB);

        // La misma regla se aplica a la velocidad. Un empuje bloqueado elimina
        // solamente la velocidad entrante del impulsor; un empuje permitido usa
        // un impulso inelástico y conserva el momento lineal sobre la normal.
        if(driver == CollisionDriver::A && !aCanPushB) {
            const float velocityIntoB =
                ilen::maths::vector::dot(
                    proxyA.motionVelocity,
                    manifold.normal
                );

            if(velocityIntoB < 0.0f) {
                proxyA.motionVelocity -= manifold.normal * velocityIntoB;

                const float rigidbodyVelocityIntoB =
                    ilen::maths::vector::dot(rigidbodyA->velocity, manifold.normal);
                if(rigidbodyVelocityIntoB < 0.0f) {
                    rigidbodyA->velocity -=
                        manifold.normal * rigidbodyVelocityIntoB;
                }
            }
        } else if(driver == CollisionDriver::B && !bCanPushA) {
            const vec2 normalB = manifold.normal * -1.0f;

            const float velocityIntoA =
                ilen::maths::vector::dot(
                    proxyB.motionVelocity,
                    normalB
                );

            if(velocityIntoA < 0.0f) {
                proxyB.motionVelocity -= normalB * velocityIntoA;

                const float rigidbodyVelocityIntoA =
                    ilen::maths::vector::dot(rigidbodyB->velocity, normalB);
                if(rigidbodyVelocityIntoA < 0.0f) {
                    rigidbodyB->velocity -=
                        normalB * rigidbodyVelocityIntoA;
                }
            }
        } else {
            const float inverseMassA = 1.0f / massA;
            const float inverseMassB = 1.0f / massB;

            const vec2 relativeVelocity =
                proxyA.motionVelocity - proxyB.motionVelocity;

            const float velocityAlongNormal =
                ilen::maths::vector::dot(
                    relativeVelocity,
                    manifold.normal
                );

            if(velocityAlongNormal < 0.0f) {
                const float impulseMagnitude =
                    -velocityAlongNormal /
                    (inverseMassA + inverseMassB);

                const vec2 impulse =
                    manifold.normal * impulseMagnitude;

                const vec2 velocityChangeA = impulse * inverseMassA;
                const vec2 velocityChangeB = impulse * -inverseMassB;

                proxyA.motionVelocity += velocityChangeA;
                proxyB.motionVelocity += velocityChangeB;

                const float rigidbodyApproachA = std::max(
                    0.0f,
                    -ilen::maths::vector::dot(
                        rigidbodyA->velocity,
                        manifold.normal
                    )
                );
                const float rigidbodyApproachB = std::max(
                    0.0f,
                    ilen::maths::vector::dot(
                        rigidbodyB->velocity,
                        manifold.normal
                    )
                );
                const float commandedApproachA = std::max(
                    0.0f,
                    -ilen::maths::vector::dot(
                        proxyA.commandedVelocity,
                        manifold.normal
                    )
                );
                const float commandedApproachB = std::max(
                    0.0f,
                    ilen::maths::vector::dot(
                        proxyB.commandedVelocity,
                        manifold.normal
                    )
                );

                // Un empuje originado principalmente por Translate/MovePosition
                // dura sólo este paso. Las colisiones nacidas de velocity o de
                // un impulso sí conservan el intercambio de momento.
                const bool transientImpulse =
                    (driver == CollisionDriver::A &&
                     commandedApproachA > rigidbodyApproachA) ||
                    (driver == CollisionDriver::B &&
                     commandedApproachB > rigidbodyApproachB);

                constexpr float commandEpsilon = 0.0001f;
                if(!transientImpulse && std::abs(ilen::maths::vector::dot(
                    proxyA.commandedVelocity,
                    manifold.normal
                )) <= commandEpsilon) {
                    rigidbodyA->velocity += velocityChangeA;
                }

                if(!transientImpulse && std::abs(ilen::maths::vector::dot(
                    proxyB.commandedVelocity,
                    manifold.normal
                )) <= commandEpsilon) {
                    rigidbodyB->velocity += velocityChangeB;
                }
            }
        }

        if(manifold.normal.y > 0.5f) {
            rigidbodyA->isInGround = true;
        }

        if(manifold.normal.y < -0.5f) {
            rigidbodyB->isInGround = true;
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

        const float velocityA = ilen::maths::vector::dot(proxyA.motionVelocity, manifold.normal);
        if(velocityA < 0.0f) {
            proxyA.motionVelocity -= manifold.normal * velocityA;

            const float rigidbodyVelocityA =
                ilen::maths::vector::dot(rigidbodyA->velocity, manifold.normal);
            if(rigidbodyVelocityA < 0.0f) {
                rigidbodyA->velocity -= manifold.normal * rigidbodyVelocityA;
            }
        }

        const vec2 normalB = manifold.normal * -1.0f;
        const float velocityB = ilen::maths::vector::dot(proxyB.motionVelocity, normalB);

        if(velocityB < 0.0f) {
            proxyB.motionVelocity -= normalB * velocityB;

            const float rigidbodyVelocityB =
                ilen::maths::vector::dot(rigidbodyB->velocity, normalB);
            if(rigidbodyVelocityB < 0.0f) {
                rigidbodyB->velocity -= normalB * rigidbodyVelocityB;
            }
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
    // CCD y el solver posicional pueden visitar una pareja varias veces durante
    // un fixed update. Se conserva un único manifold por pareja para que la API
    // Enter/Stay/Exit no reciba contactos duplicados.
    if(auto* collisionA = componentsArray->collisionComponentArray->getData(manifold.entityA)) {
        auto iterator = std::find_if(
            collisionA->contacts.begin(),
            collisionA->contacts.end(),
            [&manifold](const physics::ContactManifold2D& contact) {
                return contact.entityB == manifold.entityB;
            }
        );

        if(iterator == collisionA->contacts.end()) {
            collisionA->contacts.push_back(manifold);
        } else {
            *iterator = manifold;
        }
    }

    if(auto* collisionB = componentsArray->collisionComponentArray->getData(manifold.entityB)) {
        physics::ContactManifold2D mirrored = manifold;
        mirrored.entityA = manifold.entityB;
        mirrored.entityB = manifold.entityA;
        mirrored.normal = manifold.normal * -1.0f;

        auto iterator = std::find_if(
            collisionB->contacts.begin(),
            collisionB->contacts.end(),
            [&mirrored](const physics::ContactManifold2D& contact) {
                return contact.entityB == mirrored.entityB;
            }
        );

        if(iterator == collisionB->contacts.end()) {
            collisionB->contacts.push_back(std::move(mirrored));
        } else {
            *iterator = std::move(mirrored);
        }
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

void PhysicsSystem2D::reportCollisionDebugStats() {
    m_collisionDebugFrameCounter++;

    if(m_collisionDebugFrameCounter < m_collisionDebugReportInterval) {
        return;
    }

    m_collisionDebugFrameCounter = 0;

    const size_t pairs = m_collisionDebugStats.totalPairs();
    const size_t passedAABB = m_collisionDebugStats.aabbTests - m_collisionDebugStats.rejectedByAABB;

    const size_t avoidedPairs = 
        m_collisionDebugStats.bruteForcePairs >= m_collisionDebugStats.broadPhaseCandidates
        ? m_collisionDebugStats.bruteForcePairs - m_collisionDebugStats.broadPhaseCandidates
        : 0;

    const float reduction = 
        m_collisionDebugStats.bruteForcePairs > 0
        ? 100.0f * static_cast<float>(avoidedPairs) / static_cast<float>(m_collisionDebugStats.bruteForcePairs)
        : 0.0f;

 /*  ILEN_LOG_P(
        "PHYSICS_2D_STATS",
        "Fuerza bruta=%zu | Broadphase=%zu | "
        "Descartados=%zu | Reduccion=%.2f%% | "
        "Capas=%zu | AABB=%zu | SAT=%zu | "
        "Colisiones=%zu | Pasadas=%zu",
        m_collisionDebugStats.bruteForcePairs,
        m_collisionDebugStats.broadPhaseCandidates,
        avoidedPairs,
        reduction,
        m_collisionDebugStats.rejectedByLayer,
        m_collisionDebugStats.aabbTests,
        m_collisionDebugStats.satTests,
        m_collisionDebugStats.collisionDetected,
        m_collisionDebugStats.collisionPasses
    );
    ILEN_LOG_P(
        "PHYSICS_2D_STATS",
        "Candidatos que superaron AABB=%zu",
        passedAABB
    );*/
}
#endif
