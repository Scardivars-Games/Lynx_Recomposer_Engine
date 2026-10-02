#include <cstdlib>
#include <ilen/physics/narrow_phase_2d.h>
#include <ilen/maths/AABB.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>


namespace ilen::physics {
    using ilen::ecs::components::physics::ContactManifold2D;
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

            if(penetration < minimumPenetration) {
                minimumPenetration = penetration;
                minimumNormal = normal;
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

    std::optional<ContactManifold2D> NarrowPhase2D::Sweep(
        const ColliderProxy2D& proxyA,
        const ColliderProxy2D& proxyB
    ) {
        constexpr float epsilon = 0.00001f;

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

        if(!TestAxes(proxyA, proxyB, minimumPenetration, minimumNormal)) {
            return std::nullopt;
        }

        if(!TestAxes(proxyB, proxyA, minimumPenetration, minimumNormal)) {
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

        return manifoild;
    }
}
