#include <cstdlib>
#include <ilen/physics/narrow_phase_2d.h>
#include <ilen/maths/AABB.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>


namespace ilen::physics {
    using ilen::ecs::components::physics::ContactManifold2D;
    using ilen::ecs::components::physics::Containment2D;
    using ilen::maths::vector::vec2;

    NarrowPhase2D::Projection NarrowPhase2D::Project (
        const std::vector<vec2>& vertices,
        const vec2& axis
    ) {
        Projection projection{};

        projection.minimum = ilen::maths::vector::dot(vertices[0], axis);
        projection.maximum = projection.minimum;

        for(size_t i = 1; i < vertices.size(); i++) {
            const float value = ilen::maths::vector::dot(vertices[i], axis);
            projection.minimum = std::min(projection.minimum, value);
            projection.maximum = std::max(projection.maximum, value);
        }

        return projection;
    }

    bool NarrowPhase2D::TestAxes(
        const ColliderProxy2D& axisSource,
        const ColliderProxy2D& other,
        bool axisSourceIsA,
        float& minimumPenetration,
        vec2& minimumNormal
    ) {
        constexpr float epsilon = 0.00001f;
        const auto& vertices = axisSource.worldVertices;

        for(size_t i = 0; i < vertices.size(); i++){
            const vec2& start = vertices[i];
            const vec2& end = vertices[(i + 1) % vertices.size()];

            const vec2 edge = end - start;

            const float lenghtSquared = edge.x * edge.x + edge.y * edge.y;

            if(lenghtSquared <= epsilon) {
                continue;
            }

            const float inverseLenght = 1.0f / std::sqrt(lenghtSquared);

            const vec2 axis {
                -edge.y * inverseLenght,
                edge.x * inverseLenght
            };

            const Projection projectionA = Project(axisSource.worldVertices, axis);
            const Projection projectionB = Project(other.worldVertices, axis);

            if(projectionA.maximum <= projectionB.minimum || projectionB.maximum <= projectionA.minimum) {
                return false;
            } 

            //Calculo de la cantidad necesaria para mover A en la direccion + o - del eje.
            //Funciona incluse cuando un intervalo se encuentra dentro del otro.

            const float pushPositive = projectionB.maximum - projectionA.minimum;
            const float pushNegative = projectionA.maximum - projectionB.minimum;

            float penetration = 0.0f;
            vec2 normal{0.0f};

            if(pushPositive < pushNegative) {
                penetration = pushPositive;
                normal = axis;
            } else {
                penetration = pushNegative;
                normal = axis * -1.0f;
            }

            // TestAxes obtiene primero el MTV del dueño de los ejes respecto
            // al otro polígono. El contrato del manifold, sin embargo, exige
            // siempre la dirección en la que entityA debe salir de entityB.
            // Cuando los ejes provienen de B hay que espejar la normal. Sin
            // esto, una cara de B elegida como eje mínimo empujaba A hacia el
            // interior del sólido y generaba cascadas de depenetración.
            if(!axisSourceIsA) {
                normal *= -1.0f;
            }

            if(penetration < minimumPenetration) {
                minimumPenetration = penetration;
                minimumNormal = normal;
            }
        }

        return true;
    }

    bool NarrowPhase2D::Contains(
        const ColliderProxy2D& container,
        const ColliderProxy2D& contained
    ) {
        constexpr float containmentEpsilon = 0.00001f;
        constexpr float edgeEpsilonSquared = 0.0000000001f;
        const auto& vertices = container.worldVertices;

        // Para un polígono convexo, un segundo polígono está contenido si su
        // proyección queda dentro de la del contenedor sobre todas las normales
        // de las caras del contenedor. No depende del sentido de sus vértices.
        for(size_t index = 0; index < vertices.size(); ++index) {
            const vec2 edge =
                vertices[(index + 1) % vertices.size()] - vertices[index];
            const float lengthSquared =
                ilen::maths::vector::dot(edge, edge);

            if(lengthSquared <= edgeEpsilonSquared) {
                continue;
            }

            const float inverseLength = 1.0f / std::sqrt(lengthSquared);
            const vec2 axis{
                -edge.y * inverseLength,
                 edge.x * inverseLength
            };

            const Projection containerProjection =
                Project(container.worldVertices, axis);
            const Projection containedProjection =
                Project(contained.worldVertices, axis);

            if(
                containedProjection.minimum <
                    containerProjection.minimum - containmentEpsilon ||
                containedProjection.maximum >
                    containerProjection.maximum + containmentEpsilon
            ) {
                return false;
            }
        }

        return true;
    }

    vec2 NarrowPhase2D::AverageSupportPoint(
        const std::vector<vec2>& vertices,
        const vec2& direction
    ) {
        constexpr float supportEpsilon = 0.0001f;

        float maximumProjection = -std::numeric_limits<float>::max();

        for(const vec2& vertex: vertices) {
            maximumProjection = std::max(maximumProjection, ilen::maths::vector::dot(vertex, direction));
        }

        vec2 accumulated{0.0f};
        size_t supportCount = 0;

        for(const vec2& vertex: vertices) {
            const float projection = ilen::maths::vector::dot(vertex, direction);

            if(std::abs(projection - maximumProjection) <= supportEpsilon) {
                accumulated += vertex;
                supportCount++;
            }
        }

        if(supportCount == 0) {
            return vec2{0.0f};
        }

        return accumulated / static_cast<float>(supportCount);
    }

    vec2 NarrowPhase2D::CalculateContactPoint(
        const ColliderProxy2D& proxyA,
        const ColliderProxy2D& proxyB,
        const vec2& normal 
    ) {
        //La normal apunta hacia donde A debe salir de B.
        //La cara de A que toca B está en -normal.
        ///La cara de B que toca A está en normal.
        const vec2 pointA = AverageSupportPoint(proxyA.worldVertices, normal * -1.0f);
        const vec2 pointB = AverageSupportPoint(proxyB.worldVertices, normal);

        return (pointA + pointB) * 0.5f;
    }

    vec2 NarrowPhase2D::CalculateSweptContactPoint(
        const ColliderProxy2D& proxyA,
        const ColliderProxy2D& proxyB,
        const vec2& normal,
        float timeOfImpact
    ) {
        // Los support points se calculan en la posición inicial y después se
        // trasladan hasta el instante exacto del impacto.
        const vec2 pointA =
            AverageSupportPoint(proxyA.worldVertices, normal * -1.0f) +
            proxyA.stepTranslation * timeOfImpact;

        const vec2 pointB =
            AverageSupportPoint(proxyB.worldVertices, normal) +
            proxyB.stepTranslation * timeOfImpact;

        return (pointA + pointB) * 0.5f;
    }

    void NarrowPhase2D::UpdateProxyAtTime(
        const ColliderProxy2D& proxy,
        float time,
        ColliderProxy2D& pose
    ) {
        pose.worldVertices.resize(proxy.worldVertices.size());

        const float radians = proxy.stepRotation * time;
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        const vec2 translation = proxy.stepTranslation * time;

        for(size_t index = 0; index < proxy.worldVertices.size(); ++index) {
            const vec2& vertex = proxy.worldVertices[index];
            const vec2 local = vertex - proxy.rotationPivot;
            const vec2 rotated{
                local.x * cosine - local.y * sine,
                local.x * sine + local.y * cosine
            };

            pose.worldVertices[index] =
                proxy.rotationPivot + translation + rotated;
        }

        pose.rotationPivot = proxy.rotationPivot + translation;
        pose.stepTranslation = vec2{0.0f};
        pose.stepRotation = 0.0f;
    }

    NarrowPhase2D::SeparationResult NarrowPhase2D::FindMaximumSeparation(
        const ColliderProxy2D& proxyA,
        const ColliderProxy2D& proxyB
    ) {
        constexpr float epsilon = 0.00001f;

        SeparationResult result{
            .distance = -std::numeric_limits<float>::max(),
            .normal = vec2{0.0f}
        };

        const auto testAxes = [&](const ColliderProxy2D& axisSource) {
            const auto& vertices = axisSource.worldVertices;

            for(size_t index = 0; index < vertices.size(); ++index) {
                const vec2 edge =
                    vertices[(index + 1) % vertices.size()] - vertices[index];
                const float lengthSquared = ilen::maths::vector::dot(edge, edge);

                if(lengthSquared <= epsilon * epsilon) {
                    continue;
                }

                const float inverseLength = 1.0f / std::sqrt(lengthSquared);
                const vec2 axis{
                    -edge.y * inverseLength,
                    edge.x * inverseLength
                };

                const Projection projectionA = Project(proxyA.worldVertices, axis);
                const Projection projectionB = Project(proxyB.worldVertices, axis);

                const float bAfterA =
                    projectionB.minimum - projectionA.maximum;
                if(bAfterA > result.distance) {
                    result.distance = bAfterA;
                    result.normal = axis * -1.0f;
                }

                const float aAfterB =
                    projectionA.minimum - projectionB.maximum;
                if(aAfterB > result.distance) {
                    result.distance = aAfterB;
                    result.normal = axis;
                }
            }
        };

        testAxes(proxyA);
        testAxes(proxyB);
        return result;
    }

    std::optional<ContactManifold2D> NarrowPhase2D::SweepWithRotation(
        const ColliderProxy2D& proxyA,
        const ColliderProxy2D& proxyB
    ) {
        constexpr float epsilon = 0.00001f;
        constexpr float advanceSafety = 0.8f;
        constexpr size_t minimumIterations = 128;
        constexpr size_t maximumIterationsLimit = 1024;
        constexpr size_t refinementIterations = 20;
        constexpr float maximumTouchingAngularStep =
            1.0f * static_cast<float>(M_PI) / 180.0f;

        if(proxyA.worldVertices.size() < 3 || proxyB.worldVertices.size() < 3) {
            return std::nullopt;
        }

        // Las penetraciones iniciales pertenecen al SAT discreto.
        if(Test(proxyA, proxyB).has_value()) {
            return std::nullopt;
        }

        const vec2 relativeTranslation =
            proxyA.stepTranslation - proxyB.stepTranslation;
        const float angularTravel =
            std::abs(proxyA.stepRotation) * proxyA.maximumRadius +
            std::abs(proxyB.stepRotation) * proxyB.maximumRadius;
        const float totalAngularRotation =
            std::abs(proxyA.stepRotation) +
            std::abs(proxyB.stepRotation);

        // Cuando los polígonos comienzan tocándose, el avance conservador no
        // tiene una separación positiva desde la cual estimar el siguiente
        // instante. Reservamos suficientes iteraciones para recorrer el giro
        // completo en pasos de un grado, con un margen para los avances por
        // separación. El límite evita trabajo sin cota ante órdenes absurdas.
        const size_t angularIterationRequirement = static_cast<size_t>(
            std::ceil(totalAngularRotation / maximumTouchingAngularStep)
        ) + 64;
        const size_t maximumIterations = std::clamp(
            angularIterationRequirement,
            minimumIterations,
            maximumIterationsLimit
        );

        if(
            ilen::maths::vector::dot(relativeTranslation, relativeTranslation) <=
                epsilon * epsilon &&
            angularTravel <= epsilon
        ) {
            return std::nullopt;
        }

        float time = 0.0f;
        float previousTime = 0.0f;
        ColliderProxy2D poseA = proxyA;
        ColliderProxy2D poseB = proxyB;
        ColliderProxy2D middleA = proxyA;
        ColliderProxy2D middleB = proxyB;

        for(size_t iteration = 0; iteration < maximumIterations; ++iteration) {
            UpdateProxyAtTime(proxyA, time, poseA);
            UpdateProxyAtTime(proxyB, time, poseB);

            auto overlap = Test(poseA, poseB);
            if(overlap.has_value()) {
                float low = previousTime;
                float high = time;
                std::optional<ContactManifold2D> refined = std::move(overlap);

                for(size_t refinement = 0;
                    refinement < refinementIterations;
                    ++refinement
                ) {
                    const float middle = (low + high) * 0.5f;
                    UpdateProxyAtTime(proxyA, middle, middleA);
                    UpdateProxyAtTime(proxyB, middle, middleB);
                    auto middleOverlap = Test(middleA, middleB);

                    if(middleOverlap.has_value()) {
                        high = middle;
                        refined = std::move(middleOverlap);
                    } else {
                        low = middle;
                    }
                }

                refined->penetration = 0.0f;
                // `high` es la primera pose solapada y se conserva para
                // obtener una normal y un punto de contacto estables. El
                // movimiento, sin embargo, debe terminar en `low`, la última
                // pose libre. Colocar el proxy en `high` introducía una
                // penetración nueva cada fixed tick y trasladaba el problema
                // al solver posicional.
                refined->timeOfImpact = low;
                refined->usedAngularSweep = true;
                return refined;
            }

            if(time >= 1.0f) {
                break;
            }

            const SeparationResult separation =
                FindMaximumSeparation(poseA, poseB);

            const float closingSpeedBound =
                std::abs(ilen::maths::vector::dot(
                    relativeTranslation,
                    separation.normal
                )) + angularTravel;

            if(closingSpeedBound <= epsilon) {
                return std::nullopt;
            }

            float advance = 0.0f;

            if(separation.distance > epsilon) {
                advance =
                    (separation.distance / closingSpeedBound) * advanceSafety;
            } else {
                // En contacto exacto no existe distancia que dividir. Se usa
                // como máximo un grado angular para distinguir penetración de
                // movimiento tangencial o de separación sin saltarse un giro.
                advance = totalAngularRotation > epsilon
                    ? std::min(
                        1.0f / 128.0f,
                        maximumTouchingAngularStep / totalAngularRotation
                    )
                    : 1.0f / 128.0f;
            }

            if(advance <= epsilon) {
                advance = epsilon;
            }

            previousTime = time;
            time = std::min(1.0f, time + advance);
        }

        return std::nullopt;
    }

    bool NarrowPhase2D::AreTouching(
        const ColliderProxy2D& proxyA,
        const ColliderProxy2D& proxyB,
        float tolerance
    ) {
        if(proxyA.worldVertices.size() < 3 || proxyB.worldVertices.size() < 3) {
            return false;
        }

        return FindMaximumSeparation(proxyA, proxyB).distance <= tolerance;
    }

    std::optional<ContactManifold2D> NarrowPhase2D::Sweep(
        const ColliderProxy2D& proxyA,
        const ColliderProxy2D& proxyB
    ) {
        constexpr float epsilon = 0.00001f;

        if(
            std::abs(proxyA.stepRotation) > epsilon ||
            std::abs(proxyB.stepRotation) > epsilon
        ) {
            return SweepWithRotation(proxyA, proxyB);
        }

        if(proxyA.worldVertices.size() < 3 || proxyB.worldVertices.size() < 3) {
            return std::nullopt;
        }

        const vec2 relativeTranslation =
            proxyA.stepTranslation - proxyB.stepTranslation;

        const float movementSquared =
            relativeTranslation.x * relativeTranslation.x +
            relativeTranslation.y * relativeTranslation.y;

        if(movementSquared <= epsilon * epsilon) {
            return std::nullopt;
        }

        float globalEntry = 0.0f;
        float globalExit = 1.0f;
        vec2 collisionNormal{0.0f};
        bool separatedAtStart = false;
        bool collisionNormalFound = false;

        const auto testAxes = [&](const ColliderProxy2D& axisSource) {
            const auto& vertices = axisSource.worldVertices;

            for(size_t index = 0; index < vertices.size(); ++index) {
                const vec2 edge =
                    vertices[(index + 1) % vertices.size()] - vertices[index];

                const float lengthSquared = edge.x * edge.x + edge.y * edge.y;
                if(lengthSquared <= epsilon * epsilon) {
                    continue;
                }

                const float inverseLength = 1.0f / std::sqrt(lengthSquared);
                const vec2 axis{
                    -edge.y * inverseLength,
                    edge.x * inverseLength
                };

                const Projection projectionA = Project(proxyA.worldVertices, axis);
                const Projection projectionB = Project(proxyB.worldVertices, axis);

                const bool separatedOnAxis =
                    projectionA.maximum <= projectionB.minimum ||
                    projectionB.maximum <= projectionA.minimum;

                separatedAtStart = separatedAtStart || separatedOnAxis;

                const float axisVelocity =
                    ilen::maths::vector::dot(relativeTranslation, axis);

                if(std::abs(axisVelocity) <= epsilon) {
                    // No existe movimiento capaz de cerrar este eje separado.
                    if(separatedOnAxis) {
                        return false;
                    }
                    continue;
                }

                const float time0 =
                    (projectionB.minimum - projectionA.maximum) / axisVelocity;
                const float time1 =
                    (projectionB.maximum - projectionA.minimum) / axisVelocity;

                const float axisEntry = std::min(time0, time1);
                const float axisExit = std::max(time0, time1);

                if(
                    axisEntry > globalEntry ||
                    (!collisionNormalFound && axisEntry >= globalEntry - epsilon)
                ) {
                    globalEntry = axisEntry;
                    // La normal siempre apunta hacia donde A debe salir de B.
                    collisionNormal = axisVelocity > 0.0f
                        ? axis * -1.0f
                        : axis;
                    collisionNormalFound = true;
                }

                globalExit = std::min(globalExit, axisExit);
                if(globalEntry > globalExit + epsilon) {
                    return false;
                }
            }

            return true;
        };

        if(!testAxes(proxyA) || !testAxes(proxyB)) {
            return std::nullopt;
        }

        // Las penetraciones existentes se resuelven con SAT discreto para poder
        // obtener profundidad. Sweep solamente representa impactos futuros.
        if(!separatedAtStart) {
            return std::nullopt;
        }

        if(
            !collisionNormalFound ||
            globalExit < 0.0f ||
            globalEntry > 1.0f + epsilon
        ) {
            return std::nullopt;
        }

        const float timeOfImpact = std::clamp(globalEntry, 0.0f, 1.0f);

        ContactManifold2D manifold{};
        manifold.entityA = proxyA.entity;
        manifold.entityB = proxyB.entity;
        manifold.normal = collisionNormal;
        manifold.penetration = 0.0f;
        manifold.timeOfImpact = timeOfImpact;
        manifold.point = CalculateSweptContactPoint(
            proxyA,
            proxyB,
            collisionNormal,
            timeOfImpact
        );
        manifold.hasContactPoint = true;
        manifold.isTrigger = proxyA.isTrigger || proxyB.isTrigger;

        return manifold;
    }

    std::optional<ContactManifold2D> NarrowPhase2D::Test(const ColliderProxy2D &proxyA, const ColliderProxy2D &proxyB) {
        if(proxyA.worldVertices.size() < 3 || proxyB.worldVertices.size() < 3) {
            return std::nullopt;
        }

        /*if(!ilen::maths::aabb::overlapAABB2D(proxyA.bounds, proxyB.bounds)) {
            return std::nullopt;
        }*/

        float minimumPenetration = std::numeric_limits<float>::max();
        vec2 minimumNormal{0.0f};

        if(!TestAxes(
            proxyA,
            proxyB,
            true,
            minimumPenetration,
            minimumNormal
        )) {
            return std::nullopt;
        }

        if(!TestAxes(
            proxyB,
            proxyA,
            false,
            minimumPenetration,
            minimumNormal
        )) {
            return std::nullopt;
        }

        ContactManifold2D manifoild{};
        manifoild.entityA = proxyA.entity;
        manifoild.entityB = proxyB.entity;
        manifoild.normal = minimumNormal;
        manifoild.penetration = minimumPenetration;
        manifoild.point = CalculateContactPoint(proxyA, proxyB, minimumNormal);

        manifoild.hasContactPoint = true;

        manifoild.timeOfImpact = 1.0f;
        manifoild.isTrigger = proxyA.isTrigger || proxyB.isTrigger;

        const bool aInsideB = Contains(proxyB, proxyA);
        const bool bInsideA = Contains(proxyA, proxyB);

        // Polígonos coincidentes se contienen mutuamente, pero ninguno posee
        // un interior inequívoco respecto al otro. Se resuelven como un
        // solapamiento ordinario para conservar la simetría.
        if(aInsideB != bInsideA) {
            manifoild.containment = aInsideB
                ? Containment2D::AInsideB
                : Containment2D::BInsideA;
        }

        return manifoild;
    }
}
