#pragma once

#include <optional>

#include <ilen/physics/components/physics_components.h>
#include <ilen/transform/transform_component.h>
#include <ilen/ecs/types.h>

namespace ilen::physics {
    using namespace ilen::ecs::components::physics;
    using namespace ilen::ecs::components;
    using vec2 = ilen::maths::vector::vec2;
    class ColliderProxyBuilder2D {
        public:
            static std::optional<ColliderProxy2D> FromBox(
                ilen::ecs::Entity entity,
                const TransformComponent& transform,
                const Rigidbody2D& rigidbody,
                const BoxCollider2D& collider,
                const EntityMetadataComponent& metadata
            );

            static std::optional<ColliderProxy2D> FromPolygon(
                ilen::ecs::Entity entity,
                const TransformComponent& transform,
                const Rigidbody2D& rigidbody,
                const ConvexPolygonCollider2D& collider,
                const EntityMetadataComponent& metadata
            );

            static bool IsConvexCounterClockwise (
                const std::vector<vec2>& vertices
            );

        private:
            static vec2 TransformLocalPoint(
                const vec2& localPoint,
                const TransformComponent& transform
            );

            static ilen::maths::aabb::AABB2D CalculateBounds(
                const std::vector<vec2>& vertices
            );

            static float CalculateMaximumRadius(
                const std::vector<vec2>& vertices,
                const vec2& pivot
            );
    };
}
