#pragma once

#include <optional>
#include <vector>

#include <ilen/physics/components/physics_components.h>


namespace ilen::physics {
    using vec2 = ilen::maths::vector::vec2;

    class NarrowPhase2D final {
        private:
            struct Projection {
                float minimum = 0.0f;
                float maximum = 0.0f;
            };

            static Projection Project(
                const std::vector<vec2>& vertices, const vec2& axis
            );

            static bool TestAxes (
                const ColliderProxy2D& axisSource,
                const ColliderProxy2D& other,
                float& minimumPenetration,
                vec2& minimumNormal
            );

            static vec2 AverageSupportPoint(
                const std::vector<vec2>& vertices, const vec2& direction
            );

            static vec2 CalculateContactPoint(
                const ColliderProxy2D& proxyA,
                const ColliderProxy2D& proxyB,
                const vec2& normal
            );

            public:
                static std::optional<ecs::components::physics::ContactManifold2D> Test(
                    const ColliderProxy2D& proxyA,
                    const ColliderProxy2D& proxyB
                );
    };
}