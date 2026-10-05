#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ilen/physics/components/physics_components.h>

namespace ilen::physics {
    /** Pareja potencial. El broadphase no garantiza una colisión exacta. */
    struct BroadPhasePair2D {
        ColliderProxy2D* proxyA = nullptr;
        ColliderProxy2D* proxyB = nullptr;
    };

    /**
     * Broadphase de rejilla uniforme.
     *
     * Los estáticos permanecen cacheados hasta que cambia su colección. Los
     * dinámicos se insertan en cada subpaso usando su AABB barrido. Una pareja
     * puede coincidir en varias celdas, por eso se deduplica mediante Entity.
     */
    class BroadPhase2D final {
        private:
            struct CellCoordinate {
                int32_t x = 0;
                int32_t y = 0;

                bool operator==(const CellCoordinate& other) const {
                    return x == other.x && y == other.y;
                }
            };

            struct CellCoordinateHash {
                size_t operator()(const CellCoordinate& coordinate) const {
                    const size_t hashX = std::hash<int32_t>{}(coordinate.x);
                    const size_t hashY = std::hash<int32_t>{}(coordinate.y);

                    return hashX ^(hashY + 0x9e3779b9 + (hashX << 6) + (hashX >> 2));
                }
            };

            using CellMap = std::unordered_map<CellCoordinate, std::vector<ColliderProxy2D*>, CellCoordinateHash>;

        public:
            explicit BroadPhase2D(float cellSize = 4.0f);
            void RebuildStaticGrid(std::vector<ColliderProxy2D>& staticProxies);
            const std::vector<BroadPhasePair2D>& BuildCandidatesPairs(std::vector<ColliderProxy2D>& dynamicProxies);
            float GetCellSize() const {return m_cellSize; }
        private:
            float m_cellSize = 4.0f;
            float m_inverseCellSize = 0.25f;

            CellMap m_staticCells;
            CellMap m_dynamicCells;

            std::unordered_set<uint64_t> m_pairKeys;
            std::vector<BroadPhasePair2D> m_candidatePairs;

            // Ejecuta callback para todas las celdas tocadas por un AABB. floor
            // es importante para que las coordenadas negativas sean correctas.
            template<typename Callback>
            void ForEachOccupiedCell(const ilen::maths::aabb::AABB2D& bounds, Callback&& callback) const {
                const auto minimum = bounds.min();
                const auto maximum = bounds.max();

                const int32_t minimumX = static_cast<int32_t>(std::floor(minimum.x * m_inverseCellSize));
                const int32_t minimumY = static_cast<int32_t>(std::floor(minimum.y * m_inverseCellSize));

                const int32_t maximumX = static_cast<int32_t>(std::floor(maximum.x * m_inverseCellSize));
                const int32_t maximumY = static_cast<int32_t>(std::floor(maximum.y * m_inverseCellSize));

                for(int32_t y = minimumY; y <= maximumY; y++) {
                    for(int32_t x = minimumX; x <= maximumX; x++) {
                        callback(CellCoordinate{x, y});
                    }
                }
            }

            void TryAddPair(ColliderProxy2D* proxyA, ColliderProxy2D* proxyB);
            static uint64_t MakePairKey (ilen::ecs::Entity entityA, ilen::ecs::Entity entityB);
    };
}
