#pragma once

#include <optional>

#include <lynx/physics/components/physics_components.h>
#include <lynx/transform/transform_component.h>
#include <lynx/ecs/types.h>

namespace lynx::physics {
    using namespace lynx::ecs::components::physics;
    using namespace lynx::ecs::components;
    using vec2 = lynx::maths::vector::vec2;
    class ColliderProxyBuilder2D {
        public:
            static std::optional<ColliderProxy2D> FromBox(
                lynx::ecs::Entity entity,
                const TransformComponent& transform,
                const Rigidbody2D& rigidbody,
                const BoxCollider2D& collider,
                const EntityMetadataComponent& metadata
            );

            static std::optional<ColliderProxy2D> FromPolygon(
                lynx::ecs::Entity entity,
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

            static lynx::maths::aabb::AABB2D CalculateBounds(
                const std::vector<vec2>& vertices
            );

            static float CalculateMaximumRadius(
                const std::vector<vec2>& vertices,
                const vec2& pivot
            );
    };
}
