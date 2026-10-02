#include <ilen/physics/collider_proxy_builder_2d.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace ilen::physics {
    using namespace ilen::ecs;
    using namespace ilen::ecs::components;
    using namespace ilen::ecs::components::physics;
    using namespace ilen::maths::aabb;
    using namespace ilen::maths::vector;

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
        proxy.shape = ILEN_COLLIDER_SHAPE_TYPE_2D::Box;
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
        if (collider.vertices.size() < 3) {
            ILEN_ERROR_P("COLLIDER_PROXY_BUILDER", "Se recibio un poligono con menos de 3 vertices");
            return std::nullopt;
        }
        
        if(!IsConvexCounterClockwise(collider.vertices)) {
            ILEN_ERROR_P("COLLIDER_PROXY_BUILDER", "La figura no es convexa, o las caras no estan en el orden correcto");
            return std::nullopt;
        }

        ColliderProxy2D proxy{};
        proxy.entity = entity;
        proxy.shape = ILEN_COLLIDER_SHAPE_TYPE_2D::ConvexPolygon;
        proxy.worldVertices.reserve(collider.vertices.size());

        for(const vec2& vertex : collider.vertices) {
            proxy.worldVertices.push_back(
                TransformLocalPoint(vertex + collider.offset, transform)
            );
        }

        proxy.bounds = CalculateBounds(proxy.worldVertices);
        proxy.sweptBounds = proxy.bounds;

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

    bool ColliderProxyBuilder2D::IsConvexCounterClockwise(const std::vector<vec2>& vertices){
        if(vertices.size() < 3) {
            return false;
        }

        constexpr float epsilon = 0.00001f;
        bool foundPositiveCross = false;

        for(size_t i = 0; i < vertices.size(); i++) {
            const vec2& a = vertices[i];
            const vec2& b = vertices[(i + 1) % vertices.size()];
            const vec2& c = vertices[(i + 2) % vertices.size()];

            const vec2 edgeAB = b - a;
            const vec2 edgeBC = c - b;

            const float cross = edgeAB.x * edgeBC.y - edgeAB.y * edgeBC.x;

            /*
             * En orden antihorario todos los giros deben ser
             * positivos o colineales.
             */
            if(cross < -epsilon) {
                return false;
            }

            if(cross > epsilon) {
                foundPositiveCross = true;
            }
        }

        return foundPositiveCross;
    }
}
