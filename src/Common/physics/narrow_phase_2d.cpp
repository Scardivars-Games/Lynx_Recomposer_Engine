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

    std::optional<ContactManifold2D> NarrowPhase2D::Test(const ColliderProxy2D &proxyA, const ColliderProxy2D &proxyB) {
        if(proxyA.worldVertices.size() < 3 || proxyB.worldVertices.size() < 3) {
            return std::nullopt;
        }

        if(!ilen::maths::aabb::overlapAABB2D(proxyA.bounds, proxyB.bounds)) {
            return std::nullopt;
        }

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

        manifoild.timeOfImpact = 1.0f;
        manifoild.isTrigger = proxyA.isTrigger || proxyB.isTrigger;

        return manifoild;
    }
}