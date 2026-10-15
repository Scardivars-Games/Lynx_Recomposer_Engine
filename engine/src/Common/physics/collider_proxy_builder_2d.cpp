#include <lynx/physics/collider_proxy_builder_2d.h>
#include <lynx/physics/collider_validation_2d.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace lynx::physics {
    using namespace lynx::ecs;
    using namespace lynx::ecs::components;
    using namespace lynx::ecs::components::physics;
    using namespace lynx::maths::aabb;
    using namespace lynx::maths::vector;

    namespace {
        constexpr float DegreesToRadians = M_PI / 180.0f;

        //Se hace el ajuste de -rotation, para que el motor de físicas use la misma convención que el motor visual.
        float TransformRotationToRadians(float rotationInDegrees) {
            return -rotationInDegrees * DegreesToRadians;
        }

        vec2 RotatePoint(
            const vec2& point,
            float radians
        ) {
            const float cosine = std::cos(radians);
            const float sine = std::sin(radians);

            return vec2 {
                point.x * cosine - point.y * sine,
                point.x * sine + point.y * cosine
            };
        }
    }

    std::optional<ColliderProxy2D> ColliderProxyBuilder2D::FromBox(
        Entity entity,
        const TransformComponent& transform,
        const Rigidbody2D& rigidbody,
        const BoxCollider2D& collider,
        const EntityMetadataComponent& metadata
    ) {
        if(
            !std::isfinite(collider.halfExtents.x) ||
            !std::isfinite(collider.halfExtents.y) ||
            !std::isfinite(collider.offset.x) ||
            !std::isfinite(collider.offset.y) ||
            !std::isfinite(transform.position.x) ||
            !std::isfinite(transform.position.y) ||
            !std::isfinite(transform.rotation.z) ||
            !std::isfinite(transform.scale.x) ||
            !std::isfinite(transform.scale.y)
        ) {
            LYNX_ERROR_P(
                "COLLIDER_PROXY_BUILDER",
                "La caja o su Transform contiene valores no finitos"
            );
            return std::nullopt;
        }

        const vec2 halfExtents {
            std::abs(collider.halfExtents.x * transform.scale.x),
            std::abs(collider.halfExtents.y * transform.scale.y)
        };

        if(halfExtents.x <= 0.0f || halfExtents.y <= 0.0f) {
            return std::nullopt; //El collider no existe
        }

        const float rotation =  TransformRotationToRadians(transform.rotation.z);
        const vec2 scaledOffset {
            collider.offset.x * transform.scale.x, collider.offset.y * transform.scale.y
        };

        const vec2 center = static_cast<vec2>(transform.position) + RotatePoint(scaledOffset, rotation);
        const vec2 right = RotatePoint(vec2{halfExtents.x, 0.0f}, rotation);
        const vec2 up = RotatePoint(vec2{0.0f, halfExtents.y}, rotation);

        ColliderProxy2D proxy{};
        proxy.entity = entity;
        proxy.shape = LYNX_COLLIDER_SHAPE_TYPE_2D::Box;
        proxy.worldVertices.reserve(4);

        /*
         * Orden antihorario:
         *
         * 3 ───── 2
         * │       │
         * 0 ───── 1
         */
        proxy.worldVertices.push_back(center - right - up);
        proxy.worldVertices.push_back(center + right - up);
        proxy.worldVertices.push_back(center + right + up);
        proxy.worldVertices.push_back(center - right + up);

        proxy.bounds = CalculateBounds(proxy.worldVertices);
        proxy.sweptBounds = proxy.bounds;
        proxy.rotationPivot = static_cast<vec2>(transform.position);
        proxy.maximumRadius = CalculateMaximumRadius(
            proxy.worldVertices,
            proxy.rotationPivot
        );

        proxy.previousCenter = rigidbody.prevPosition + RotatePoint(scaledOffset, rotation);

        proxy.isStatic = rigidbody.isStatic;
        proxy.isKinematic = rigidbody.isKinematic && !rigidbody.isStatic;
        proxy.isTrigger = collider.isTrigger;
        proxy.layerMask = static_cast<uint32_t>(metadata.layers);

        return proxy;
    }

    std::optional<ColliderProxy2D> ColliderProxyBuilder2D::FromPolygon(
        Entity entity,
        const TransformComponent& transform,
        const Rigidbody2D& rigidbody,
        const ConvexPolygonCollider2D& collider,
        const EntityMetadataComponent& metadata
    ) {
        constexpr float scaleEpsilon = 0.00001f;

        if (collider.vertices.size() < 3) {
            LYNX_ERROR_P("COLLIDER_PROXY_BUILDER", "Se recibio un poligono con menos de 3 vertices");
            return std::nullopt;
        }

        if(
            !std::isfinite(collider.offset.x) ||
            !std::isfinite(collider.offset.y) ||
            !std::isfinite(transform.position.x) ||
            !std::isfinite(transform.position.y) ||
            !std::isfinite(transform.rotation.z) ||
            !std::isfinite(transform.scale.x) ||
            !std::isfinite(transform.scale.y)
        ) {
            LYNX_ERROR_P(
                "COLLIDER_PROXY_BUILDER",
                "El poligono o su Transform contiene valores no finitos"
            );
            return std::nullopt;
        }

        if(
            std::abs(transform.scale.x) <= scaleEpsilon ||
            std::abs(transform.scale.y) <= scaleEpsilon
        ) {
            LYNX_ERROR_P(
                "COLLIDER_PROXY_BUILDER",
                "Un poligono fisico no puede tener escala cero"
            );
            return std::nullopt;
        }
        
        if(!IsConvexCounterClockwise(collider.vertices)) {
            LYNX_ERROR_P("COLLIDER_PROXY_BUILDER", "La figura no es convexa, o las caras no estan en el orden correcto");
            return std::nullopt;
        }

        ColliderProxy2D proxy{};
        proxy.entity = entity;
        proxy.shape = LYNX_COLLIDER_SHAPE_TYPE_2D::ConvexPolygon;
        proxy.worldVertices.reserve(collider.vertices.size());

        for(const vec2& vertex : collider.vertices) {
            proxy.worldVertices.push_back(
                TransformLocalPoint(vertex + collider.offset, transform)
            );
        }

        // Una reflexión por escala negativa invierte el winding. Restaurarlo
        // mantiene la convención antihoraria del narrowphase.
        if(transform.scale.x * transform.scale.y < 0.0f) {
            std::reverse(
                proxy.worldVertices.begin(),
                proxy.worldVertices.end()
            );
        }

        proxy.bounds = CalculateBounds(proxy.worldVertices);
        proxy.sweptBounds = proxy.bounds;
        proxy.rotationPivot = static_cast<vec2>(transform.position);
        proxy.maximumRadius = CalculateMaximumRadius(
            proxy.worldVertices,
            proxy.rotationPivot
        );

        const float rotation = TransformRotationToRadians(transform.rotation.z);

        const vec2 scaledOffset {
            collider.offset.x * transform.scale.x,
            collider.offset.y * transform.scale.y
        };

        proxy.previousCenter = rigidbody.prevPosition + RotatePoint(scaledOffset, rotation);
        proxy.isStatic = rigidbody.isStatic;
        proxy.isKinematic = rigidbody.isKinematic && !rigidbody.isStatic;
        proxy.isTrigger = collider.isTrigger;
        proxy.layerMask = static_cast<uint32_t>(metadata.layers);

        return proxy;
    }

    vec2 ColliderProxyBuilder2D::TransformLocalPoint(
        const vec2& localPoint,
        const TransformComponent& transform
    ) {
        const vec2 scaled {
            localPoint.x * transform.scale.x,
            localPoint.y * transform.scale.y
        };

        const float rotation = TransformRotationToRadians(transform.rotation.z);

        return static_cast<vec2>(transform.position) + RotatePoint(scaled, rotation);
    }

    AABB2D ColliderProxyBuilder2D::CalculateBounds(
        const std::vector<vec2>& vertices
    ){
        vec2 minimum {
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        };

        vec2 maximum {
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest()
        };

        for (const vec2& vertex : vertices) {
            minimum.x = std::min(minimum.x, vertex.x);
            minimum.y = std::min(minimum.y, vertex.y);
            maximum.x = std::max(maximum.x, vertex.x);
            maximum.y = std::max(maximum.y, vertex.y);
        }

        return AABB2D::fromMinMax(minimum, maximum);
    }

    float ColliderProxyBuilder2D::CalculateMaximumRadius(
        const std::vector<vec2>& vertices,
        const vec2& pivot
    ) {
        float maximumRadiusSquared = 0.0f;

        for(const vec2& vertex : vertices) {
            const vec2 difference = vertex - pivot;
            maximumRadiusSquared = std::max(
                maximumRadiusSquared,
                lynx::maths::vector::dot(difference, difference)
            );
        }

        return std::sqrt(maximumRadiusSquared);
    }

    bool ColliderProxyBuilder2D::IsConvexCounterClockwise(const std::vector<vec2>& vertices){
        return IsValidConvexPolygon2D(vertices);
    }
}
