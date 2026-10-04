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

            struct SeparationResult {
                float distance = 0.0f;
                vec2 normal{0.0f};
            };

            static Projection Project(
                const std::vector<vec2>& vertices, const vec2& axis
            );

            static bool TestAxes (
                const ColliderProxy2D& axisSource,
                const ColliderProxy2D& other,
                bool axisSourceIsA,
                float& minimumPenetration,
                vec2& minimumNormal
            );

            static bool Contains(
                const ColliderProxy2D& container,
                const ColliderProxy2D& contained
            );

            static vec2 AverageSupportPoint(
                const std::vector<vec2>& vertices, const vec2& direction
            );

            static vec2 CalculateContactPoint(
                const ColliderProxy2D& proxyA,
                const ColliderProxy2D& proxyB,
                const vec2& normal
            );

            static vec2 CalculateSweptContactPoint(
                const ColliderProxy2D& proxyA,
                const ColliderProxy2D& proxyB,
                const vec2& normal,
                float timeOfImpact
            );

            static void UpdateProxyAtTime(
                const ColliderProxy2D& proxy,
                float time,
                ColliderProxy2D& pose
            );

            static SeparationResult FindMaximumSeparation(
                const ColliderProxy2D& proxyA,
                const ColliderProxy2D& proxyB
            );

            static std::optional<ecs::components::physics::ContactManifold2D>
            SweepWithRotation(
                const ColliderProxy2D& proxyA,
                const ColliderProxy2D& proxyB
            );

            public:
                /**
                 * Indica si dos polígonos están solapados o separados por una
                 * distancia menor que la tolerancia indicada.
                 */
                static bool AreTouching(
                    const ColliderProxy2D& proxyA,
                    const ColliderProxy2D& proxyB,
                    float tolerance = 0.0001f
                );

                /** SAT discreto para dos polígonos que ya ocupan su posición actual. */
                static std::optional<ecs::components::physics::ContactManifold2D> Test(
                    const ColliderProxy2D& proxyA,
                    const ColliderProxy2D& proxyB
                );

                /**
                 * SAT continuo para polígonos convexos con traslación lineal.
                 *
                 * Retorna el primer contacto en el intervalo normalizado [0, 1].
                 * No barre rotaciones y deliberadamente retorna nullopt cuando
                 * los polígonos ya comienzan solapados; ese caso pertenece a Test.
                 */
                static std::optional<ecs::components::physics::ContactManifold2D> Sweep(
                    const ColliderProxy2D& proxyA,
                    const ColliderProxy2D& proxyB
                );
    };
}
