#include <cstdint>
#include <ilen/physics/broad_phase_2d.h>
#include <algorithm>
#include <vector>
#include "ilen/physics/components/specification/collider_components.h"

namespace ilen::physics {
    BroadPhase2D::BroadPhase2D(float cellSize) {
        constexpr float minimumCellSize = 0.01f;

        m_cellSize = std::max(cellSize, minimumCellSize);
        m_inverseCellSize = 1.0f / m_cellSize;
    }

    uint64_t BroadPhase2D::MakePairKey(ilen::ecs::Entity entityA, ilen::ecs::Entity entityB) {
        const uint32_t minimum = std::min(entityA, entityB);
        const uint32_t maximum = std::max(entityA, entityB);

        return (static_cast<uint64_t>(minimum) << 32 | static_cast<uint64_t>(maximum));
    }

    void BroadPhase2D::TryAddPair(ColliderProxy2D *proxyA, ColliderProxy2D *proxyB) {
        if(!proxyA || !proxyB || proxyA == proxyB || proxyA->entity == proxyB->entity) {
            return;
        }

        const uint64_t key = MakePairKey(proxyA->entity, proxyB->entity);

        if(!m_pairKeys.insert(key).second) {
            return;
        }

        m_candidatePairs.push_back({.proxyA = proxyA, .proxyB = proxyB});
    }

    void BroadPhase2D::RebuildStaticGrid(std::vector<ColliderProxy2D> &staticProxies) {
        m_staticCells.clear();

        //Estimando celulas, para ahorar rehashes.
        m_staticCells.reserve(staticProxies.size() * 2);

        for(ColliderProxy2D& proxy: staticProxies) {
            ForEachOccupiedCell(
                proxy.bounds, 
                [this, &proxy](const CellCoordinate& coordinate) {
                    m_staticCells[coordinate].push_back(&proxy);
                }
            );
        }
    }

    const std::vector<BroadPhasePair2D>& BroadPhase2D::BuildCandidatesPairs(std::vector<ColliderProxy2D> &dynamicProxies) {
        m_dynamicCells.clear();
        m_pairKeys.clear();
        m_candidatePairs.clear();

        m_dynamicCells.reserve(dynamicProxies.size() * 2);
        m_candidatePairs.reserve(dynamicProxies.size() * 4);

        for(ColliderProxy2D& dynamicProxy: dynamicProxies) {
            ForEachOccupiedCell(
                dynamicProxy.bounds,
                [this, &dynamicProxy](const CellCoordinate& coordinate) {
                    if(auto iterator = m_dynamicCells.find(coordinate); iterator != m_dynamicCells.end()) {
                        for(ColliderProxy2D* other : iterator->second) {
                            TryAddPair(&dynamicProxy, other);
                        }
                    }

                    if(auto iterator = m_staticCells.find(coordinate); iterator != m_staticCells.end()) {
                        for(ColliderProxy2D* staticProxy : iterator->second) {
                            TryAddPair(&dynamicProxy, staticProxy);
                        }
                    }

                    m_dynamicCells[coordinate].push_back(&dynamicProxy);
                }
            );
        }

        return m_candidatePairs;
    }
}