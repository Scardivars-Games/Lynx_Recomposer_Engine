#include <algorithm>
#include <cmath>
#include <cstdint>
#include <lynx/physics/physics_2d_system.h>
#include <lynx/headers/ecs.h>
#include <lynx/headers/ecs_components.h>
#include <lynx_public/API/physics_2d_api.h>
#include <iterator>
#include <limits>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <lynx/maths/AABB.h>
#include <lynx/physics/narrow_phase_2d.h>
#include "lynx/ecs/types.h"


using namespace lynx::ecs::systems;
using namespace lynx::ecs::components;
using namespace lynx::maths::vector;

namespace {
    constexpr float DegreesToRadians2D = M_PI / 180.0f;
    constexpr float RadiansToDegrees2D = 180.0f / M_PI;

    float ShortestAngleDelta(float fromDegrees, float toDegrees) {
        float difference = std::fmod(toDegrees - fromDegrees, 360.0f);

        if(difference > 180.0f) difference -= 360.0f;
        if(difference < -180.0f) difference += 360.0f;
        return difference;
    }

}

void PhysicsSystem2D::Init(const lynx::EngineContext* engineContext){
    p_physicsOptions = &engineContext->runtimeConfiguration.get()->physics;
    p_coordinator = engineContext->p_coordinator.get();
    p_jobSystem = engineContext->p_jobSystem.get();
    p_commandDispatcher = engineContext->p_commandDispatcher.get();

    maxEntities = engineContext->runtimeConfiguration->ecs.initialEntityCapacity;

    m_dynamicEntities.reserve(maxEntities);
    m_staticProxies.reserve(maxEntities);
    m_dynamicProxies.reserve(maxEntities);
    m_rotationBlockingStatic.assign(maxEntities, NULL_ENTITY);
    m_registrationStates.assign(maxEntities, RegistrationState::None);
    m_requestedTranslations.assign(maxEntities, vec2(0.0f));
    m_teleportTargets.assign(maxEntities, vec2(0.0f));
    m_hasTeleport.assign(maxEntities, false);
    m_requestedRotations.assign(maxEntities, 0.0f);
    m_teleportRotationTargets.assign(maxEntities, 0.0f);
    m_hasTeleportRotation.assign(maxEntities, false);
    m_activeVelocityAxes.assign(maxEntities, std::uint8_t{0});
    m_rotationBlockingStatic.assign(maxEntities, NULL_ENTITY);
    
#if LYNX_DEBUG
    m_staticGuards.reserve(maxEntities);
    m_persistentDebugImpacts.reserve(4096);
    m_debugActiveVectors.reserve(maxEntities * 3);
    m_debugAppliedForces.assign(maxEntities, vec2(0.0f));
#endif

    p_coordinator->registerComponent<physics::Rigidbody2D>();
    p_coordinator->registerComponent<physics::BoxCollider2D>();
    p_coordinator->registerComponent<physics::ConvexPolygonCollider2D>();
    p_coordinator->registerComponent<physics::CollisionComponent2D>();

    lynx::engine::api::PhysicsAPI::Init(engineContext);
}


//TODO: hacer este sistema multihilo.
void PhysicsSystem2D::Update() {
    lynx::time::ScopedTimer timer("Physics 2D update");
   
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
    std::fill(m_activeVelocityAxes.begin(), m_activeVelocityAxes.end(), 0);

#if LYNX_DEBUG
    // Los vectores verdes describen únicamente el fixed update actual.
    m_debugActiveVectors.clear();
    std::fill(m_debugAppliedForces.begin(), m_debugAppliedForces.end(), vec2(0.0f));
#endif


    syncEntities(&componentArray);

    //if(m_dynamicEntities.size() + m_staticEntities.size() != entities.size())
    if(m_dynamicEntities.empty() && m_staticProxies.empty()) return;

    const float dt = static_cast<float>(1.0 / p_physicsOptions->updatedPerSecond);

    prepareCollisionData(&componentArray);
    registerPrevPosition(&componentArray);
    applyForces(dt, &componentArray);
    applyGravity(dt, &componentArray);
    applyMovementResistance(dt, &componentArray);
    rebuildDynamicProxies(dt, &componentArray);
    integrate(dt, &componentArray);
    detectAndResolve(dt, &componentArray);
    refreshRotationBlocks();

#if LYNX_DEBUG
    if(p_physicsOptions->showDebugArrows) {
        rebuildDebugActiveVectors(&componentArray);
    }
    reportCollisionDebugStats();
#endif

    for(auto& [id, order] : m_accumulatedTransformOrder) {
        auto* pendingOrder = componentArray.transformOrderArray->getData(id);

        if(pendingOrder && pendingOrder->hasOrders) {
            // Puede haber más de un fixed update dentro del mismo frame de
            // render. TransformSystem todavía no consumió el resultado del
            // paso anterior, por lo que hay que acumularlo en vez de
            // reemplazarlo. Sumar funciona tanto para órdenes relativas como
            // para un destino absoluto pendiente: en el segundo caso desplaza
            // directamente dicho destino.
            pendingOrder->translation += order.translation;
            pendingOrder->rotation += order.rotation;
            pendingOrder->hasOrders = true;
        } else {
            order.hasOrders = true;
            componentArray.transformOrderArray->updateData(
                id,
                std::move(order)
            );
        }
    }

#if LYNX_DEBUG
    checkStaticGuard(&componentArray);
#endif
}

void PhysicsSystem2D::registerEntity(Entity e, ArrayOfComponentsUsed* componentsArray) {
    auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
    auto* boxColider = componentsArray->boxColliderArray->getData(e);
    auto* polygonCollider = componentsArray->convexPolygonCollider->getData(e);

    if(!rb) return;

    m_rotationBlockingStatic[e] = NULL_ENTITY;

    if((boxColider == nullptr) == (polygonCollider == nullptr)) {
        LYNX_ERROR_P("PHYSICS_2D", "La entidad %u debe tener al menos un collider 2D", e);
        return;
    }

    //Método de seguridad para forzar el collider si este no existe.
    if(boxColider && boxColider->halfExtents.x == 0.0f && boxColider->halfExtents.y == 0.0f) {
        lynx::engine::api::PhysicsAPI::GenerateBoxColliderSize(e, boxColider);
    }

    auto proxy = buildProxy(e, componentsArray);

    if(!proxy.has_value()) {
        return;
    }

    // Toda entidad que participa válidamente en las físicas 2D conserva un
    // CollisionComponent2D durante toda su vida física. El manifold cambia en
    // cada fixed update, pero la presencia del componente es estructural: sirve
    // como punto de comunicación para sistemas ECS y como firma estable para el
    // puente de eventos hacia GameObject.
    if(!componentsArray->collisionComponentArray->getData(e)) {
        p_coordinator->setComponent(
            e,
            physics::CollisionComponent2D{}
        );
    }

    if(rb->isStatic) {
        m_staticProxies.push_back(std::move(*proxy));
        m_staticBroadPhaseDirty = true;
        m_registrationStates[e] = RegistrationState::Static;

#if LYNX_DEBUG
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
        m_rotationBlockingStatic[entity] = NULL_ENTITY;
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
#if LYNX_DEBUG
        m_staticGuards[i] = std::move(m_staticGuards.back());
        m_staticGuards.pop_back();
#endif
    }

    for(Entity entity: entities) {
        if(entity >= maxEntities) {
            continue;
        }

        if(m_registrationStates[entity] == RegistrationState::None) {
            registerEntity(entity, componentsArray);
        }
    }
}

void PhysicsSystem2D::unregisterEntity(Entity e){
    if(e >= maxEntities) {
        return;
    }

    const RegistrationState state = m_registrationStates[e];
    m_rotationBlockingStatic[e] = NULL_ENTITY;

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
            [e](const lynx::physics::ColliderProxy2D& proxy) {
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

#if LYNX_DEBUG
            m_staticGuards[index] = std::move(m_staticGuards.back());
            m_staticGuards.pop_back();
#endif
        }
    }

    m_registrationStates[e] = RegistrationState::None;
}

std::optional<lynx::physics::ColliderProxy2D> PhysicsSystem2D::buildProxy(
    Entity entity, ArrayOfComponentsUsed* componentsArray
) {
    auto* transform = componentsArray->transformComponentArray->getData(entity);
    auto* pendingOrder = componentsArray->transformOrderArray->getData(entity);
    auto* rb = componentsArray->rigidbodyComponentArray->getData(entity);
    auto* metadata = componentsArray->entityMetadataArray->getData(entity);

    if(!transform || !rb || !metadata) {
        return std::nullopt;
    }

    const TransformComponent simulatedTransform = effectiveTransform(
        *transform,
        pendingOrder
    );

    if(auto* box = componentsArray->boxColliderArray->getData(entity)) {
        return lynx::physics::ColliderProxyBuilder2D::FromBox(
            entity, 
            simulatedTransform,
            *rb,
            *box,
            *metadata
        );
    }

    if(auto* polygon = componentsArray->convexPolygonCollider->getData(entity)) {
        return lynx::physics::ColliderProxyBuilder2D::FromPolygon(
            entity,
            simulatedTransform,
            *rb,
            *polygon,
            *metadata
        );
    }

    return std::nullopt;
}

TransformComponent PhysicsSystem2D::effectiveTransform(
    const TransformComponent& transform,
    const TransformOrder* pendingOrder
) {
    // Sólo se copian los datos que necesita ColliderProxyBuilder2D. Evitamos
    // copiar childrens y el resto del estado jerárquico del componente.
    TransformComponent result{};
    result.position = transform.position;
    result.rotation = transform.rotation;
    result.scale = transform.scale;

    if(!pendingOrder || !pendingOrder->hasOrders) {
        return result;
    }

    result.position = pendingOrder->isNewPos
        ? pendingOrder->translation
        : result.position + pendingOrder->translation;

    result.rotation = pendingOrder->isNewRot
        ? pendingOrder->rotation
        : result.rotation + pendingOrder->rotation;

    if(pendingOrder->isNewScale) {
        result.scale = pendingOrder->scaleMultiplier;
    } else {
        result.scale.x *= pendingOrder->scaleMultiplier.x;
        result.scale.y *= pendingOrder->scaleMultiplier.y;
        result.scale.z *= pendingOrder->scaleMultiplier.z;
    }

    return result;
}

void PhysicsSystem2D::registerPrevPosition(ArrayOfComponentsUsed* componentsArray){
    for(Entity e: m_dynamicEntities){
        auto* transform = componentsArray->transformComponentArray->getData(e);
        auto* pendingOrder = componentsArray->transformOrderArray->getData(e);
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);

        if(!transform || !rb) {
            continue;
        }

        const TransformComponent simulatedTransform = effectiveTransform(
            *transform,
            pendingOrder
        );

        // Guardamos la memoria del frame anterior
        rb->prevPosition = static_cast<vec2>(simulatedTransform.position);
    }
}

void PhysicsSystem2D::prepareCollisionData(
    ArrayOfComponentsUsed* componentsArray
) {
    const auto prepareContacts = [](physics::CollisionComponent2D* collision) {
        if(!collision) {
            return;
        }

        // Un Exit publicado en el paso anterior ya cumplió su ciclo de vida.
        // Los contactos restantes se marcan provisionalmente como Exit: si el
        // narrowphase vuelve a encontrarlos, writeContacts los convierte en
        // Stay. Un contacto nuevo se insertará directamente como Enter.
        std::erase_if(
            collision->contacts,
            [](const physics::ContactManifold2D& contact) {
                return contact.phase == physics::CollisionPhase2D::Exit;
            }
        );

        for(auto& contact : collision->contacts) {
            contact.phase = physics::CollisionPhase2D::Exit;
        }
    };

    for(Entity entity : m_dynamicEntities) {
        prepareContacts(
            componentsArray->collisionComponentArray->getData(entity)
        );
    }

    for(const auto& proxy : m_staticProxies) {
        prepareContacts(
            componentsArray->collisionComponentArray->getData(proxy.entity)
        );
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
        auto* pendingOrder =
            componentArray->transformOrderArray->getData(entity);

        if(!rigidbody || !transform) {
            continue;
        }

        const TransformComponent simulatedTransform = effectiveTransform(
            *transform,
            pendingOrder
        );

        const bool rotationBlocked =
            m_rotationBlockingStatic[entity] != NULL_ENTITY;

        // Teleport cambia el origen del proxy sin barrer el trayecto. El SAT
        // discreto del solver resolverá cualquier contacto en el destino.
        if(m_hasTeleport[entity]) {
            const vec2 teleportDelta =
                m_teleportTargets[entity] -
                static_cast<vec2>(simulatedTransform.position);

            translateProxy(*proxy, teleportDelta);
            m_accumulatedTransformOrder[entity].translation +=
                static_cast<vec3>(teleportDelta);
        }

        if(m_hasTeleportRotation[entity] && !rotationBlocked) {
            const float visualRotationDelta =
                m_teleportRotationTargets[entity] -
                simulatedTransform.rotation.z;

            rotateProxy(
                *proxy,
                -visualRotationDelta * DegreesToRadians2D
            );
            m_accumulatedTransformOrder[entity].rotation.z +=
                visualRotationDelta;
        }

        proxy->commandedVelocity = dt > 0.0f
            ? m_requestedTranslations[entity] / dt
            : vec2(0.0f);

        proxy->motionVelocity =
            rigidbody->velocity + proxy->commandedVelocity;

        proxy->motionAngularVelocity = dt > 0.0f && !rotationBlocked
            ? -m_requestedRotations[entity] * DegreesToRadians2D / dt
            : 0.0f;

        // El proxy comienza en la posición actual o teletransportada. La
        // integración continua lo avanzará hasta el primer impacto.
        m_dynamicProxies.push_back(std::move(*proxy));
    }
}

void PhysicsSystem2D::translateProxy(
    lynx::physics::ColliderProxy2D& proxy, const vec2& translation
) {
    for(vec2& vertex : proxy.worldVertices) {
        vertex += translation;
    }

    proxy.bounds.center += translation;
    proxy.rotationPivot += translation;
}

void PhysicsSystem2D::rotateProxy(
    lynx::physics::ColliderProxy2D& proxy,
    float radians
) {
    if(radians == 0.0f) {
        return;
    }

    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    vec2 minimum{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };
    vec2 maximum{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    };

    for(vec2& vertex : proxy.worldVertices) {
        const vec2 local = vertex - proxy.rotationPivot;
        vertex = proxy.rotationPivot + vec2{
            local.x * cosine - local.y * sine,
            local.x * sine + local.y * cosine
        };

        minimum = lynx::maths::vector::min(minimum, vertex);
        maximum = lynx::maths::vector::max(maximum, vertex);
    }

    proxy.bounds = AABB2D::fromMinMax(minimum, maximum);
}

AABB2D PhysicsSystem2D::calculateSweptBounds(
    const lynx::physics::ColliderProxy2D& proxy,
    const vec2& translation
) {
    constexpr float angularEpsilon = 0.000001f;

    if(std::abs(proxy.stepRotation) > angularEpsilon) {
        const vec2 endPivot = proxy.rotationPivot + translation;
        const vec2 radius{proxy.maximumRadius};

        return AABB2D::fromMinMax(
            lynx::maths::vector::min(proxy.rotationPivot, endPivot) - radius,
            lynx::maths::vector::max(proxy.rotationPivot, endPivot) + radius
        );
    }

    AABB2D endBounds = proxy.bounds;
    endBounds.center += translation;

    return AABB2D::fromMinMax(
        lynx::maths::vector::min(proxy.bounds.min(), endBounds.min()),
        lynx::maths::vector::max(proxy.bounds.max(), endBounds.max())
    );
}

void PhysicsSystem2D::applyForces(float dt, ArrayOfComponentsUsed* componentsArray){
    constexpr uint8_t xAxis = 1u << 0u;
    constexpr uint8_t yAxis = 1u << 1u;

    for(Entity e : m_dynamicEntities){
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
        auto* order = componentsArray->physicsOrderArray->getData(e);

        m_requestedTranslations[e] = vec2(0.0f);
        m_teleportTargets[e] = vec2(0.0f);
        m_hasTeleport[e] = false;
        m_requestedRotations[e] = 0.0f;
        m_teleportRotationTargets[e] = 0.0f;
        m_hasTeleportRotation[e] = false;

        if(rb && order && order->hasOrders){
            // Las velocidades pedidas por la API deben existir antes de
            // calcular fuerzas y gravedad para que el CCD barra exactamente
            // el movimiento que ejecutará este fixed update.
            if(order->overrideVelocityX) {
                rb->velocity.x = order->velocityOverride.x;
                m_activeVelocityAxes[e] |= xAxis;
            }

            if(order->overrideVelocityY) {
                rb->velocity.y = order->velocityOverride.y;
                m_activeVelocityAxes[e] |= yAxis;
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

            if(order->hasTeleportRotation) {
                m_teleportRotationTargets[e] = order->targetRotation;
                m_hasTeleportRotation[e] = true;
            } else if(order->hasRotateTo && transform) {
                m_requestedRotations[e] = ShortestAngleDelta(
                    transform->rotation.z,
                    order->targetRotation
                );
            }

            m_requestedRotations[e] += order->accumulatedRotation;

            if(!rb->isKinematic) {
                const vec2 force = static_cast<vec2>(order->accumulatedForce);
                const vec2 impulse = static_cast<vec2>(order->accumulatedImpulse);

                // La resistencia se decide por eje. Una fuerza vertical no
                // debe impedir que se elimine una velocidad fantasma en X, ni
                // una entrada horizontal debe alterar la caída en Y.
                if(force.x != 0.0f || impulse.x != 0.0f) {
                    m_activeVelocityAxes[e] |= xAxis;
                }

                if(force.y != 0.0f || impulse.y != 0.0f) {
                    m_activeVelocityAxes[e] |= yAxis;
                }

                //Calculamos la aceleracion: F = m * a => a = F / m
                vec2 acceleration = force / rb->mass;
                rb->velocity += acceleration * dt;

                //Añadimos los impulsos. Estos no dependen de dt.
                rb->velocity += impulse / rb->mass;

#if LYNX_DEBUG
                // Un impulso es instantáneo. Para poder compararlo visualmente
                // con una fuerza se representa como su fuerza media en este
                // fixed update: F = J / dt.
                if(p_physicsOptions->showDebugArrows) {
                    m_debugAppliedForces[e] +=
                        static_cast<vec2>(order->accumulatedForce);
                    if(dt > 0.0f) {
                        m_debugAppliedForces[e] +=
                            static_cast<vec2>(order->accumulatedImpulse) / dt;
                    }
                }
#endif
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
            order->accumulatedRotation = 0.0f;
            order->targetRotation = 0.0f;
            order->hasRotateTo = false;
            order->hasTeleportRotation = false;
            order->hasOrders = false;
        }
    }
}

void PhysicsSystem2D::applyGravity(float dt, ArrayOfComponentsUsed* componentsArray){
    constexpr uint8_t yAxis = 1u << 1u;

    for(Entity e: m_dynamicEntities) {
        auto* rb = componentsArray->rigidbodyComponentArray->getData(e);
        if(!rb || rb->isKinematic || !rb->usesGravity /*|| rb->isInGround*/) continue; //Es increible que desactivar el calculo de la gravedad si se toca el cielo hace colapsar todo, XD.

        // La gravedad es una aceleración continua sobre Y; ese eje no debe
        // amortiguarse mientras esté habilitada.
        m_activeVelocityAxes[e] |= yAxis;
        rb->velocity.y += p_physicsOptions->gravity * rb->gravityScale * dt;

#if LYNX_DEBUG
        // F = m * a. Guardarla antes del solver permite ver que la gravedad
        // sigue solicitándose aunque el contacto con el suelo la rechace.
        if(p_physicsOptions->showDebugArrows) {
            m_debugAppliedForces[e].y +=
                rb->mass * p_physicsOptions->gravity *
                rb->gravityScale;
        }
#endif

        //Clamp a la velocidad máxima vertical del motor (tanto si va para arriba como abajo)
        if(std::abs(rb->velocity.y) > rb->terminalVelocity) {
            rb->velocity.y = (rb->velocity.y > 0) ? rb->terminalVelocity : -rb->terminalVelocity;
        }
    }//Entidades estáticas por concepto no pueden ser afectadas por la gravedad
}

void PhysicsSystem2D::applyMovementResistance(
    float dt,
    ArrayOfComponentsUsed* componentsArray
) {
    constexpr uint8_t xAxis = 1u << 0u;
    constexpr uint8_t yAxis = 1u << 1u;

    const float resistance = std::max(
        0.0f,
        p_physicsOptions->linearMovementResistance
    );
    const float zeroThreshold = std::max(
        0.0f,
        p_physicsOptions->velocityZeroThreshold
    );

    // v(t + dt) = v(t) * e^(-resistance * dt). Esta forma produce el mismo
    // decaimiento por segundo con 30, 60 o 120 pasos físicos, a diferencia de
    // multiplicar por un porcentaje fijo en cada frame.
    const float dampingFactor = std::exp(-resistance * dt);

    for(Entity entity : m_dynamicEntities) {
        auto* rigidbody =
            componentsArray->rigidbodyComponentArray->getData(entity);

        // Los cinemáticos obedecen órdenes de movimiento y no deben perder
        // velocidad por una simulación física que deliberadamente no los rige.
        if(!rigidbody || rigidbody->isStatic || rigidbody->isKinematic) {
            continue;
        }

        const uint8_t activeAxes = m_activeVelocityAxes[entity];

        if((activeAxes & xAxis) == 0u) {
            rigidbody->velocity.x *= dampingFactor;
        }

        if((activeAxes & yAxis) == 0u) {
            rigidbody->velocity.y *= dampingFactor;
        }

        // El clamp también respeta los ejes activos: una velocidad pequeña
        // solicitada intencionalmente no debe convertirse en cero.
        if(
            (activeAxes & xAxis) == 0u &&
            std::abs(rigidbody->velocity.x) < zeroThreshold
        ) {
            rigidbody->velocity.x = 0.0f;
        }

        if(
            (activeAxes & yAxis) == 0u &&
            std::abs(rigidbody->velocity.y) < zeroThreshold
        ) {
            rigidbody->velocity.y = 0.0f;
        }
    }
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
            proxy.stepRotation = 0.0f;
            proxy.sweptBounds = proxy.bounds;
            continue;
        }

        proxy.stepTranslation = proxy.motionVelocity * remainingTime;
        proxy.stepRotation = proxy.motionAngularVelocity * remainingTime;
        proxy.sweptBounds = calculateSweptBounds(
            proxy,
            proxy.stepTranslation
        );
    }
}

void PhysicsSystem2D::advanceDynamicProxies(float timeFraction) {
    for(auto& proxy : m_dynamicProxies) {
        const vec2 translation = proxy.stepTranslation * timeFraction;
        const float rotation = proxy.stepRotation * timeFraction;

        if(
            translation.x == 0.0f &&
            translation.y == 0.0f &&
            rotation == 0.0f
        ) {
            proxy.sweptBounds = proxy.bounds;
            continue;
        }

        m_accumulatedTransformOrder[proxy.entity].translation +=
            static_cast<vec3>(translation);
        m_accumulatedTransformOrder[proxy.entity].rotation.z +=
            -rotation * RadiansToDegrees2D;

        rotateProxy(proxy, rotation);
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

#if LYNX_DEBUG
    m_collisionDebugStats.reset();
    m_debugCurrentCollisionPairs.clear();
    m_debugFixedDeltaTime =
        p_physicsOptions->showDebugArrows ? dt : 0.0f;
#endif

    if(m_staticBroadPhaseDirty) {
        m_broadPhase.RebuildStaticGrid(m_staticProxies);
        m_staticBroadPhaseDirty = false;
    }

    float remainingTime = dt;
    std::unordered_set<uint64_t> stalledPenetrationPairs;
    std::unordered_map<uint64_t, uint32_t> penetrationStallCounts;

    const auto makePairKey = [](Entity entityA, Entity entityB) {
        const uint32_t minimum = std::min(entityA, entityB);
        const uint32_t maximum = std::max(entityA, entityB);
        return
            (static_cast<uint64_t>(minimum) << 32) |
            static_cast<uint64_t>(maximum);
    };

    for(
        uint32_t eventIndex = 0;
        eventIndex < maximumCollisionEvents && remainingTime > minimumRemainingTime;
        ++eventIndex
    ) {
        prepareSweeps(remainingTime, componentsArray);
        const auto& candidatePairs =
            m_broadPhase.BuildCandidatesPairs(m_dynamicProxies);

#if LYNX_DEBUG
        const size_t dynamicCount = m_dynamicProxies.size();
        const size_t staticCount = m_staticProxies.size();
        m_collisionDebugStats.collisionPasses++;
        m_collisionDebugStats.bruteForcePairs +=
            (dynamicCount * (dynamicCount > 0 ? dynamicCount - 1 : 0)) / 2 +
            dynamicCount * staticCount;
        m_collisionDebugStats.broadPhaseCandidates += candidatePairs.size();
#endif

        struct ContactEvent {
            lynx::physics::ColliderProxy2D* proxyA = nullptr;
            lynx::physics::ColliderProxy2D* proxyB = nullptr;
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

            if(stalledPenetrationPairs.contains(
                makePairKey(proxyA.entity, proxyB.entity)
            )) {
                continue;
            }

#if LYNX_DEBUG
            if(proxyB.isStatic) {
                m_collisionDebugStats.dynamicVsStaticPairs++;
            } else {
                m_collisionDebugStats.dynamicVsDynamicPairs++;
            }
#endif

            const auto layerA =
                static_cast<lynx::enumerations::LYNX_LAYER_SYSTEM>(proxyA.layerMask);
            const auto layerB =
                static_cast<lynx::enumerations::LYNX_LAYER_SYSTEM>(proxyB.layerMask);

            if(!lynx::physics::CollisionMatrix::CanCollide(p_physicsOptions->collisionMatrix, layerA, layerB)) {
#if LYNX_DEBUG
                m_collisionDebugStats.rejectedByLayer++;
#endif
                continue;
            }

#if LYNX_DEBUG
            m_collisionDebugStats.aabbTests++;
#endif

            const bool overlapsNow = lynx::maths::aabb::overlapAABB2D(
                proxyA.bounds,
                proxyB.bounds
            );

            if(overlapsNow) {
#if LYNX_DEBUG
                m_collisionDebugStats.satTests++;
#endif

                auto manifold = lynx::physics::NarrowPhase2D::Test(proxyA, proxyB);

                if(manifold.has_value()) {
#if LYNX_DEBUG
                    m_collisionDebugStats.collisionDetected++;
#endif

                    if(manifold->isTrigger) {
                        writeContacts(*manifold, componentsArray);
                    } else {
                        constexpr float angularMotionEpsilon = 0.000001f;

                        if(
                            !proxyA.isStatic && proxyB.isStatic &&
                            std::abs(proxyA.motionAngularVelocity) >
                                angularMotionEpsilon
                        ) {
                            m_rotationBlockingStatic[proxyA.entity] =
                                proxyB.entity;
                            proxyA.motionAngularVelocity = 0.0f;
                            proxyA.stepRotation = 0.0f;
                        }

                        if(
                            proxyA.isStatic && !proxyB.isStatic &&
                            std::abs(proxyB.motionAngularVelocity) >
                                angularMotionEpsilon
                        ) {
                            m_rotationBlockingStatic[proxyB.entity] =
                                proxyA.entity;
                            proxyB.motionAngularVelocity = 0.0f;
                            proxyB.stepRotation = 0.0f;
                        }

                        penetrations.push_back({
                            .proxyA = &proxyA,
                            .proxyB = &proxyB,
                            .manifold = std::move(*manifold)
                        });
                    }

                    continue;
                }
            }

            if(!lynx::maths::aabb::overlapAABB2D(proxyA.sweptBounds, proxyB.sweptBounds)) {
#if LYNX_DEBUG
                m_collisionDebugStats.rejectedByAABB++;
#endif
                continue;
            }

#if LYNX_DEBUG
            m_collisionDebugStats.satTests++;
#endif

            auto manifold = lynx::physics::NarrowPhase2D::Sweep(proxyA, proxyB);
            if(!manifold.has_value()) {
                continue;
            }

#if LYNX_DEBUG
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

        // Todas las penetraciones se detectaron sobre la misma fotografía del
        // mundo. Resolver una modifica los proxies, por lo que los manifolds
        // restantes quedan obsoletos. Una pasada Gauss-Seidel recalcula cada
        // pareja justo antes de corregirla. No se repite aquí: una figura
        // encajada puede alternar entre paredes opuestas y acumular una gran
        // traslación aunque ninguna corrección individual sea grande.
        if(!penetrations.empty()) {
            constexpr uint32_t penetrationSolverIterations = 1;
            constexpr uint32_t penetrationStallThreshold = 3;
            constexpr float penetrationProgressEpsilon = 0.00001f;
            std::unordered_map<uint64_t, float> penetrationBeforeCorrection;
            penetrationBeforeCorrection.reserve(penetrations.size());

            for(
                uint32_t iteration = 0;
                iteration < penetrationSolverIterations;
                ++iteration
            ) {
                bool foundPenetration = false;

                for(auto& contact : penetrations) {
                    auto currentManifold = lynx::physics::NarrowPhase2D::Test(
                        *contact.proxyA,
                        *contact.proxyB
                    );

                    if(!currentManifold.has_value()) {
                        continue;
                    }

                    penetrationBeforeCorrection[makePairKey(
                        contact.proxyA->entity,
                        contact.proxyB->entity
                    )] = currentManifold->penetration;

                    foundPenetration = true;
                    resolveCollision(
                        *contact.proxyA,
                        *contact.proxyB,
                        *currentManifold,
                        componentsArray
                    );
                }

                if(!foundPenetration) {
                    break;
                }
            }

            // Un objeto puede quedar geométricamente encajado entre varias
            // superficies sin que exista una corrección compatible para todas.
            // No basta una penetración residual para declararlo bloqueado: una
            // sola pasada Gauss-Seidel deja residuos normales en pilas como
            // jugador-caja-suelo. Solamente activamos la salida de emergencia
            // cuando la misma pareja dinámico-estático no mejora durante tres
            // eventos consecutivos del fixed tick. Las parejas dinámico-dinámico
            // nunca se congelan aquí porque todavía deben intercambiar movimiento.
            for(auto& contact : penetrations) {
                const uint64_t pairKey = makePairKey(
                    contact.proxyA->entity,
                    contact.proxyB->entity
                );

                auto remainingManifold = lynx::physics::NarrowPhase2D::Test(
                    *contact.proxyA,
                    *contact.proxyB
                );

                if(!remainingManifold.has_value()) {
                    penetrationStallCounts.erase(pairKey);
                    continue;
                }

                if(contact.proxyA->isStatic == contact.proxyB->isStatic) {
                    penetrationStallCounts.erase(pairKey);
                    continue;
                }

                const auto before = penetrationBeforeCorrection.find(pairKey);
                if(
                    before != penetrationBeforeCorrection.end() &&
                    remainingManifold->penetration <
                        before->second - penetrationProgressEpsilon
                ) {
                    penetrationStallCounts[pairKey] = 0;
                    continue;
                }

                uint32_t& stallCount = penetrationStallCounts[pairKey];
                ++stallCount;

                if(stallCount < penetrationStallThreshold) {
                    continue;
                }

                stalledPenetrationPairs.insert(pairKey);

                if(!contact.proxyA->isStatic) {
                    contact.proxyA->solverFrozen = true;
                    contact.proxyA->motionVelocity = vec2{0.0f};
                    contact.proxyA->motionAngularVelocity = 0.0f;
                    contact.proxyA->stepTranslation = vec2{0.0f};
                    contact.proxyA->stepRotation = 0.0f;
                }

                if(!contact.proxyB->isStatic) {
                    contact.proxyB->solverFrozen = true;
                    contact.proxyB->motionVelocity = vec2{0.0f};
                    contact.proxyB->motionAngularVelocity = 0.0f;
                    contact.proxyB->stepTranslation = vec2{0.0f};
                    contact.proxyB->stepRotation = 0.0f;
                }
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

        // La fase angular actual es cinemática: no calcula torque ni momento
        // de inercia. Cuando el TOI fue causado por rotación, se conserva el
        // ángulo alcanzado y se descarta solamente la rotación restante de
        // este fixed tick para impedir ciclos de impacto a tiempo cero.
        for(const auto& contact : earliestImpacts) {
            if(!contact.manifold.usedAngularSweep) {
                continue;
            }

            if(!contact.proxyA->isStatic) {
                if(contact.proxyB->isStatic) {
                    m_rotationBlockingStatic[contact.proxyA->entity] =
                        contact.proxyB->entity;
                }
                contact.proxyA->motionAngularVelocity = 0.0f;
                contact.proxyA->stepRotation = 0.0f;
            }

            if(!contact.proxyB->isStatic) {
                if(contact.proxyA->isStatic) {
                    m_rotationBlockingStatic[contact.proxyB->entity] =
                        contact.proxyA->entity;
                }
                contact.proxyB->motionAngularVelocity = 0.0f;
                contact.proxyB->stepRotation = 0.0f;
            }
        }

        const float remainingTimeAfterImpact =
            remainingTime * (1.0f - timeOfImpact);

        // Gauss-Seidel sobre la isla del mismo TOI. Repetir este conjunto
        // propaga en el mismo evento que la caja está bloqueada por la pared o
        // por el suelo, sin consumir ocho eventos a tiempo cero.
        constexpr uint32_t simultaneousSolverIterations = 16;
        constexpr float solvedVelocityEpsilon = 0.0001f;

        struct BlockedDirection {
            lynx::physics::ColliderProxy2D* proxy = nullptr;
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
                    lynx::enumerations::LYNX_LAYER_SYSTEM
                >(pair.proxyA->layerMask);

                const auto layerB = static_cast<
                    lynx::enumerations::LYNX_LAYER_SYSTEM
                >(pair.proxyB->layerMask);

                if(!lynx::physics::CollisionMatrix::CanCollide(p_physicsOptions->collisionMatrix, layerA, layerB)) {
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

                if(!lynx::maths::aabb::overlapAABB2D(
                    pair.proxyA->sweptBounds,
                    pair.proxyB->sweptBounds
                )) {
                    continue;
                }

#if LYNX_DEBUG
                m_collisionDebugStats.satTests++;
#endif

                auto propagatedImpact = lynx::physics::NarrowPhase2D::Sweep(
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

                if(propagatedImpact->usedAngularSweep) {
                    if(!pair.proxyA->isStatic) {
                        if(pair.proxyB->isStatic) {
                            m_rotationBlockingStatic[pair.proxyA->entity] =
                                pair.proxyB->entity;
                        }
                        pair.proxyA->motionAngularVelocity = 0.0f;
                        pair.proxyA->stepRotation = 0.0f;
                    }

                    if(!pair.proxyB->isStatic) {
                        if(pair.proxyA->isStatic) {
                            m_rotationBlockingStatic[pair.proxyB->entity] =
                                pair.proxyA->entity;
                        }
                        pair.proxyB->motionAngularVelocity = 0.0f;
                        pair.proxyB->stepRotation = 0.0f;
                    }
                }

#if LYNX_DEBUG
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
                const lynx::physics::ColliderProxy2D* proxy,
                const vec2& normal
            ) {
                return std::any_of(
                    blockedDirections.begin(),
                    blockedDirections.end(),
                    [proxy, &normal](const BlockedDirection& blocked) {
                        return blocked.proxy == proxy &&
                            lynx::maths::vector::dot(
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
                lynx::physics::ColliderProxy2D* proxy,
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
                const float normalMotion = lynx::maths::vector::dot(
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
                    lynx::maths::vector::dot(
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
                    lynx::maths::vector::dot(
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

#if LYNX_DEBUG
    if(p_physicsOptions->showDebugArrows) {
        m_debugPreviousCollisionPairs = m_debugCurrentCollisionPairs;
    } else {
        m_debugPreviousCollisionPairs.clear();
    }
#endif
}

void PhysicsSystem2D::refreshRotationBlocks() {
    constexpr float staticContactTolerance = 0.001f;

    for(const auto& dynamicProxy : m_dynamicProxies) {
        Entity& blockingStatic =
            m_rotationBlockingStatic[dynamicProxy.entity];

        if(blockingStatic == NULL_ENTITY) {
            continue;
        }

        const auto staticIterator = std::find_if(
            m_staticProxies.begin(),
            m_staticProxies.end(),
            [blockingStatic](const lynx::physics::ColliderProxy2D& proxy) {
                return proxy.entity == blockingStatic;
            }
        );

        if(
            staticIterator == m_staticProxies.end() ||
            !lynx::physics::NarrowPhase2D::AreTouching(
                dynamicProxy,
                *staticIterator,
                staticContactTolerance
            )
        ) {
            blockingStatic = NULL_ENTITY;
        }
    }
}

void PhysicsSystem2D::resolveCollision(
    lynx::physics::ColliderProxy2D& proxyA,
    lynx::physics::ColliderProxy2D& proxyB,
    const physics::ContactManifold2D& manifold,
    ArrayOfComponentsUsed* componentsArray
){
    writeContacts(manifold, componentsArray);

    if(manifold.isTrigger) {
        return;
    }

#if LYNX_DEBUG
    if(p_physicsOptions->showDebugArrows) {
        recordDebugImpact(
            proxyA,
            proxyB,
            manifold,
            componentsArray
        );
    }
#endif

    auto* rigidbodyA = componentsArray->rigidbodyComponentArray->getData(proxyA.entity);
    auto* rigidbodyB = componentsArray->rigidbodyComponentArray->getData(manifold.entityB);

    const bool dynamicA =
        rigidbodyA && !rigidbodyA->isStatic && !proxyA.solverFrozen;
    const bool dynamicB =
        rigidbodyB && !rigidbodyB->isStatic && !proxyB.solverFrozen;

    if(manifold.containment != physics::Containment2D::None) {
        // La contención es una violación más fuerte que una penetración común:
        // repartir el MTV por masas mueve también el volumen contenedor y puede
        // dejar al cuerpo pequeño todavía dentro. La salida pertenece completa
        // al cuerpo contenido. Sólo si éste es estático se mueve el contenedor.
        const bool movableA = rigidbodyA && !rigidbodyA->isStatic;
        const bool movableB = rigidbodyB && !rigidbodyB->isStatic;

        lynx::physics::ColliderProxy2D* escapingProxy = nullptr;
        physics::Rigidbody2D* escapingRigidbody = nullptr;
        vec2 escapeNormal{0.0f};

        if(manifold.containment == physics::Containment2D::AInsideB) {
            if(movableA) {
                escapingProxy = &proxyA;
                escapingRigidbody = rigidbodyA;
                escapeNormal = manifold.normal;
            } else if(movableB) {
                escapingProxy = &proxyB;
                escapingRigidbody = rigidbodyB;
                escapeNormal = manifold.normal * -1.0f;
            }
        } else {
            if(movableB) {
                escapingProxy = &proxyB;
                escapingRigidbody = rigidbodyB;
                escapeNormal = manifold.normal * -1.0f;
            } else if(movableA) {
                escapingProxy = &proxyA;
                escapingRigidbody = rigidbodyA;
                escapeNormal = manifold.normal;
            }
        }

        if(!escapingProxy || !escapingRigidbody) {
            return;
        }

        const vec2 separation = escapeNormal * manifold.penetration;
        m_accumulatedTransformOrder[escapingProxy->entity].translation +=
            static_cast<vec3>(separation);
        translateProxy(*escapingProxy, separation);

        // Se elimina únicamente la componente que vuelve a introducir el
        // cuerpo en el sólido. El movimiento tangencial y el movimiento hacia
        // la salida permanecen intactos.
        const float motionIntoSolid = lynx::maths::vector::dot(
            escapingProxy->motionVelocity,
            escapeNormal
        );
        if(motionIntoSolid < 0.0f) {
            escapingProxy->motionVelocity -=
                escapeNormal * motionIntoSolid;
        }

        const float rigidbodyMotionIntoSolid = lynx::maths::vector::dot(
            escapingRigidbody->velocity,
            escapeNormal
        );
        if(rigidbodyMotionIntoSolid < 0.0f) {
            escapingRigidbody->velocity -=
                escapeNormal * rigidbodyMotionIntoSolid;
        }

        escapingProxy->solverFrozen = false;
        if(escapeNormal.y > 0.5f) {
            escapingRigidbody->isInGround = true;
        }
        return;
    }

    if(dynamicA && !dynamicB) {
        const vec2 separation = manifold.normal * manifold.penetration;
        m_accumulatedTransformOrder[proxyA.entity].translation += static_cast<vec3>(separation);
        translateProxy(proxyA, separation);

        const float normalMotion =
            lynx::maths::vector::dot(proxyA.motionVelocity, manifold.normal);

        if(normalMotion < 0.0f) {
            proxyA.motionVelocity -= manifold.normal * normalMotion;
        }

        const float rigidbodyNormalVelocity =
            lynx::maths::vector::dot(rigidbodyA->velocity, manifold.normal);

        if(rigidbodyNormalVelocity < 0.0f) {
            rigidbodyA->velocity -=
                manifold.normal * rigidbodyNormalVelocity;
        }

        if(manifold.normal.y > 0.5f) {
            rigidbodyA->isInGround = true;
        }

        return;
    }

    if(!dynamicA && dynamicB) {
        const vec2 normalB = manifold.normal * -1.0f;
        const vec2 separation = normalB * manifold.penetration;
        m_accumulatedTransformOrder[proxyB.entity].translation +=
            static_cast<vec3>(separation);
        translateProxy(proxyB, separation);

        const float normalMotion =
            lynx::maths::vector::dot(proxyB.motionVelocity, normalB);

        if(normalMotion < 0.0f) {
            proxyB.motionVelocity -= normalB * normalMotion;
        }

        const float rigidbodyNormalVelocity =
            lynx::maths::vector::dot(rigidbodyB->velocity, normalB);

        if(rigidbodyNormalVelocity < 0.0f) {
            rigidbodyB->velocity -= normalB * rigidbodyNormalVelocity;
        }

        if(normalB.y > 0.5f) {
            rigidbodyB->isInGround = true;
        }

        return;
    }

    if(!dynamicA && !dynamicB) {
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

        const float relativeNormalVelocity = lynx::maths::vector::dot(
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
        const float relativeNormalVelocity = lynx::maths::vector::dot(
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
                if(std::abs(lynx::maths::vector::dot(
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

                if(std::abs(lynx::maths::vector::dot(
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
        p_physicsOptions->solveRealisticCollision
    ) {
        constexpr float minimumMass = 0.0001f;
        constexpr float movementEpsilon = 0.0001f;
        constexpr float momentumEpsilon = 0.0001f;
        constexpr float maximumPushRatio = 2.0f;

        const float massA = std::max(rigidbodyA->mass, minimumMass);
        const float massB = std::max(rigidbodyB->mass, minimumMass);

        const float approachA = std::max(
            0.0f,
            -lynx::maths::vector::dot(
                proxyA.motionVelocity,
                manifold.normal
            )
        );

        const float approachB = std::max(
            0.0f,
            lynx::maths::vector::dot(
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
                lynx::maths::vector::dot(
                    proxyA.motionVelocity,
                    manifold.normal
                );

            if(velocityIntoB < 0.0f) {
                proxyA.motionVelocity -= manifold.normal * velocityIntoB;

                const float rigidbodyVelocityIntoB =
                    lynx::maths::vector::dot(rigidbodyA->velocity, manifold.normal);
                if(rigidbodyVelocityIntoB < 0.0f) {
                    rigidbodyA->velocity -=
                        manifold.normal * rigidbodyVelocityIntoB;
                }
            }
        } else if(driver == CollisionDriver::B && !bCanPushA) {
            const vec2 normalB = manifold.normal * -1.0f;

            const float velocityIntoA =
                lynx::maths::vector::dot(
                    proxyB.motionVelocity,
                    normalB
                );

            if(velocityIntoA < 0.0f) {
                proxyB.motionVelocity -= normalB * velocityIntoA;

                const float rigidbodyVelocityIntoA =
                    lynx::maths::vector::dot(rigidbodyB->velocity, normalB);
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
                lynx::maths::vector::dot(
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

                const float commandedApproachA = std::max(
                    0.0f,
                    -lynx::maths::vector::dot(
                        proxyA.commandedVelocity,
                        manifold.normal
                    )
                );
                const float commandedApproachB = std::max(
                    0.0f,
                    lynx::maths::vector::dot(
                        proxyB.commandedVelocity,
                        manifold.normal
                    )
                );

                // Move/MovePosition describen un desplazamiento temporal, no
                // una velocidad física. Si cualquiera de los cuerpos produjo
                // el contacto mediante una orden, el impulso sólo modifica el
                // movimiento interno de este tick y nunca Rigidbody::velocity.
                const bool transientImpulse =
                    commandedApproachA > movementEpsilon ||
                    commandedApproachB > movementEpsilon ||
                    manifold.usedAngularSweep;

                constexpr float commandEpsilon = 0.0001f;
                if(!transientImpulse && std::abs(lynx::maths::vector::dot(
                    proxyA.commandedVelocity,
                    manifold.normal
                )) <= commandEpsilon) {
                    rigidbodyA->velocity += velocityChangeA;
                }

                if(!transientImpulse && std::abs(lynx::maths::vector::dot(
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
        const vec2 separationA =
            manifold.normal * (manifold.penetration * 0.5f);
        const vec2 separationB = separationA * -1.0f;

        m_accumulatedTransformOrder[proxyA.entity].translation += static_cast<vec3>(separationA);
        m_accumulatedTransformOrder[proxyB.entity].translation += static_cast<vec3>(separationB);

        translateProxy(proxyA, separationA);
        translateProxy(proxyB, separationB);

        const float velocityA = lynx::maths::vector::dot(proxyA.motionVelocity, manifold.normal);
        if(velocityA < 0.0f) {
            proxyA.motionVelocity -= manifold.normal * velocityA;

            const float rigidbodyVelocityA =
                lynx::maths::vector::dot(rigidbodyA->velocity, manifold.normal);
            if(rigidbodyVelocityA < 0.0f) {
                rigidbodyA->velocity -= manifold.normal * rigidbodyVelocityA;
            }
        }

        const vec2 normalB = manifold.normal * -1.0f;
        const float velocityB = lynx::maths::vector::dot(proxyB.motionVelocity, normalB);

        if(velocityB < 0.0f) {
            proxyB.motionVelocity -= normalB * velocityB;

            const float rigidbodyVelocityB =
                lynx::maths::vector::dot(rigidbodyB->velocity, normalB);
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
            physics::ContactManifold2D entered = manifold;
            entered.phase = physics::CollisionPhase2D::Enter;
            collisionA->contacts.push_back(std::move(entered));
        } else {
            const physics::CollisionPhase2D phase =
                iterator->phase == physics::CollisionPhase2D::Exit
                    ? physics::CollisionPhase2D::Stay
                    : iterator->phase;
            *iterator = manifold;
            iterator->phase = phase;
        }
    }

    if(auto* collisionB = componentsArray->collisionComponentArray->getData(manifold.entityB)) {
        physics::ContactManifold2D mirrored = manifold;
        mirrored.entityA = manifold.entityB;
        mirrored.entityB = manifold.entityA;
        mirrored.normal = manifold.normal * -1.0f;
        if(manifold.containment == physics::Containment2D::AInsideB) {
            mirrored.containment = physics::Containment2D::BInsideA;
        } else if(manifold.containment == physics::Containment2D::BInsideA) {
            mirrored.containment = physics::Containment2D::AInsideB;
        }

        auto iterator = std::find_if(
            collisionB->contacts.begin(),
            collisionB->contacts.end(),
            [&mirrored](const physics::ContactManifold2D& contact) {
                return contact.entityB == mirrored.entityB;
            }
        );

        if(iterator == collisionB->contacts.end()) {
            mirrored.phase = physics::CollisionPhase2D::Enter;
            collisionB->contacts.push_back(std::move(mirrored));
        } else {
            const physics::CollisionPhase2D phase =
                iterator->phase == physics::CollisionPhase2D::Exit
                    ? physics::CollisionPhase2D::Stay
                    : iterator->phase;
            *iterator = std::move(mirrored);
            iterator->phase = phase;
        }
    }
}

#if LYNX_DEBUG
void PhysicsSystem2D::rebuildDebugActiveVectors(
    ArrayOfComponentsUsed* componentsArray
) {
    constexpr float vectorEpsilonSquared = 0.000001f;

    for(const auto& proxy : m_dynamicProxies) {
        const auto* transform =
            componentsArray->transformComponentArray->getData(proxy.entity);
        const float z = transform ? transform->position.z + 0.02f : 0.02f;

        const float linearLengthSquared =
            lynx::maths::vector::dot(
                proxy.motionVelocity,
                proxy.motionVelocity
            );

        if(linearLengthSquared > vectorEpsilonSquared) {
            m_debugActiveVectors.push_back({
                .point = proxy.rotationPivot,
                .value = proxy.motionVelocity,
                .z = z,
                .entity = proxy.entity,
                .kind = DebugActiveVectorKind2D::LinearVelocity
            });
        }

        // La velocidad angular no posee una dirección lineal propia. Para
        // hacerla visible se dibuja la velocidad tangencial del vértice más
        // alejado del pivote: v = omega x r.
        if(std::abs(proxy.motionAngularVelocity) > 0.0001f) {
            vec2 outerPoint = proxy.rotationPivot;
            vec2 outerRadius{0.0f};
            float maximumDistanceSquared = 0.0f;

            for(const vec2& vertex : proxy.worldVertices) {
                const vec2 radius = vertex - proxy.rotationPivot;
                const float distanceSquared =
                    lynx::maths::vector::dot(radius, radius);

                if(distanceSquared > maximumDistanceSquared) {
                    maximumDistanceSquared = distanceSquared;
                    outerPoint = vertex;
                    outerRadius = radius;
                }
            }

            const vec2 tangentialVelocity{
                -outerRadius.y * proxy.motionAngularVelocity,
                 outerRadius.x * proxy.motionAngularVelocity
            };

            if(
                lynx::maths::vector::dot(
                    tangentialVelocity,
                    tangentialVelocity
                ) > vectorEpsilonSquared
            ) {
                m_debugActiveVectors.push_back({
                    .point = outerPoint,
                    .value = tangentialVelocity,
                    .z = z,
                    .entity = proxy.entity,
                    .kind = DebugActiveVectorKind2D::AngularVelocity
                });
            }
        }

        const vec2 force = m_debugAppliedForces[proxy.entity];
        if(
            lynx::maths::vector::dot(force, force) >
            vectorEpsilonSquared
        ) {
            m_debugActiveVectors.push_back({
                .point = proxy.rotationPivot,
                .value = force,
                .z = z + 0.01f,
                .entity = proxy.entity,
                .kind = DebugActiveVectorKind2D::Force
            });
        }
    }
}

void PhysicsSystem2D::recordDebugImpact(
    const lynx::physics::ColliderProxy2D& proxyA,
    const lynx::physics::ColliderProxy2D& proxyB,
    const physics::ContactManifold2D& manifold,
    ArrayOfComponentsUsed* componentsArray
) {
    if(!manifold.hasContactPoint || m_debugFixedDeltaTime <= 0.0f) {
        return;
    }

    const uint32_t minimumEntity = std::min(proxyA.entity, proxyB.entity);
    const uint32_t maximumEntity = std::max(proxyA.entity, proxyB.entity);
    const uint64_t pairKey =
        (static_cast<uint64_t>(minimumEntity) << 32) |
        static_cast<uint64_t>(maximumEntity);

    const bool firstImpactThisTick =
        m_debugCurrentCollisionPairs.insert(pairKey).second;

    if(
        !firstImpactThisTick ||
        m_debugPreviousCollisionPairs.contains(pairKey)
    ) {
        return;
    }

    const auto velocityAtContact = [&manifold](
        const lynx::physics::ColliderProxy2D& proxy
    ) {
        const vec2 radius = manifold.point - proxy.rotationPivot;
        const vec2 angularVelocity{
            -radius.y * proxy.motionAngularVelocity,
             radius.x * proxy.motionAngularVelocity
        };

        return proxy.motionVelocity + angularVelocity;
    };

    const vec2 relativeVelocity =
        velocityAtContact(proxyA) - velocityAtContact(proxyB);

    const float closingSpeed = std::max(
        0.0f,
        -lynx::maths::vector::dot(relativeVelocity, manifold.normal)
    );

    // Si el contacto ya nació penetrado, su corrección posicional también es
    // una señal importante: se expresa como la velocidad necesaria para sacar
    // el cuerpo en un único fixed tick.
    const float correctionSpeed = std::max(
        0.0f,
        manifold.penetration / m_debugFixedDeltaTime
    );

    const float impactSpeed = std::max(closingSpeed, correctionSpeed);
    if(impactSpeed <= 0.0001f) {
        return;
    }

    const auto* rigidbodyA =
        componentsArray->rigidbodyComponentArray->getData(proxyA.entity);
    const auto* rigidbodyB =
        componentsArray->rigidbodyComponentArray->getData(proxyB.entity);

    constexpr float minimumMass = 0.0001f;
    const float inverseMassA = proxyA.isStatic || !rigidbodyA
        ? 0.0f
        : 1.0f / std::max(rigidbodyA->mass, minimumMass);
    const float inverseMassB = proxyB.isStatic || !rigidbodyB
        ? 0.0f
        : 1.0f / std::max(rigidbodyB->mass, minimumMass);
    const float totalInverseMass = inverseMassA + inverseMassB;

    if(totalInverseMass <= 0.0f) {
        return;
    }

    // Impulso inelástico equivalente dividido por dt: fuerza media aplicada
    // durante el fixed tick. Se guarda sin clamp para hacer visibles explosiones
    // numéricas del solver mediante líneas deliberadamente enormes.
    const float impulse = impactSpeed / totalInverseMass;
    const float force = impulse / m_debugFixedDeltaTime;

    float z = 0.0f;
    if(const auto* transform =
        componentsArray->transformComponentArray->getData(proxyA.entity)
    ) {
        z = transform->position.z + 0.01f;
    }

    m_persistentDebugImpacts.push_back({
        .point = manifold.point,
        .direction = manifold.normal,
        .force = force,
        .z = z,
        .entityA = proxyA.entity,
        .entityB = proxyB.entity
    });
}

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
            LYNX_ERROR_P("PHYSICS_2D", "La entidad estática %u modifico su Transform", guard.entity);
        }

        if(!rigidbody->isStatic) {
            LYNX_ERROR_P("PHYSICS_2D", "La entidad %u fue registrada como estática y luego fue cambiado ese estado", guard.entity);
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

 /*  LYNX_LOG_P(
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
    LYNX_LOG_P(
        "PHYSICS_2D_STATS",
        "Candidatos que superaron AABB=%zu",
        passedAABB
    );*/
}
#endif
