#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <lynx/headers/enumerations.h>

namespace lynx::physics {
    class CollisionMatrix final {
        public:
            using Storage = std::array<std::uint32_t, 32>;

            static void SetLayerCollision(Storage& matrix, enumerations::LYNX_LAYER_SYSTEM layerA, enumerations::LYNX_LAYER_SYSTEM layerB, bool canCollide) {
                const auto indexA = GetBitIndex(layerA);
                const auto indexB = GetBitIndex(layerB);

                if(!indexA || !indexB) return;

                const auto layerABits = static_cast<std::uint32_t>(layerA);
                const auto layerBBits = static_cast<std::uint32_t>(layerB);

                if(canCollide) {
                    matrix[*indexA] |= layerABits;
                    matrix[*indexB] |= layerBBits;
                } else {
                    matrix[*indexA] &= ~layerBBits;
                    matrix[*indexB] &= ~layerABits;
                }
            }

            static bool CanCollide(const Storage& matrix, enumerations::LYNX_LAYER_SYSTEM layerA, enumerations::LYNX_LAYER_SYSTEM layerB) {
                if(layerA == enumerations::LYNX_LAYER_SYSTEM::None || layerB == enumerations::LYNX_LAYER_SYSTEM::None){
                    return false;
                }

                const auto indexA = GetBitIndex(layerA);
                const auto indexB = GetBitIndex(layerB);

                if(!indexA || !indexB) {
                    return false;
                }

                const auto layerBBits = static_cast<std::uint32_t>(layerB);

                return (matrix[*indexA] & layerBBits) != 0;
            }

        private:
            static std::optional<std::size_t> GetBitIndex(enumerations::LYNX_LAYER_SYSTEM layer) {
                const auto value = static_cast<std::uint32_t>(layer);

                if(value == 0) {
                    return std::nullopt;
                }

                if(!std::has_single_bit(value)) {
                    return std::nullopt;
                }

                return static_cast<std::size_t>(std::countr_zero(value));
            }
    };
}


/*#pragma once
#include <cstdint>
#include <lynx/headers/enumerations.h>
#include <lynx/core/runtime_configuration.h>

namespace lynx::physics {
    using namespace lynx::enumerations;
    using namespace lynx::core;
    
    class CollisionMatrix {
        private:
            static int GetBitIndex(uint32_t flag) {
                if(flag == 0) return -1;
#if defined(__GNUC__) || defined(__clang__)
                return __builtin_ctz(flag);
#elif defined(_MSC_VER)
                unsigned long index;
                _BitScanForward(&index, flag);
                return index;
#else
                int index = 0;
                while((flag & 1) == 0) {flag >>= 1; index++; }
                return index;
#endif
            }

            public:

                static void SetLayerCollision(LYNX_LAYER_SYSTEM layerA, LYNX_LAYER_SYSTEM layerB, bool canCollide){
                    int indexA = GetBitIndex(static_cast<uint32_t>(layerA));
                    int indexB = GetBitIndex(static_cast<uint32_t>(layerB));

                    if(indexA < 0 || indexB < 0) return;

                    if(canCollide){
                        PhysicsOptions::collisionMatrix[indexA] |= static_cast<uint32_t>(layerB);
                        PhysicsOptions::collisionMatrix[indexB] |= static_cast<uint32_t>(layerA);
                    } else {
                        PhysicsOptions::collisionMatrix[indexA] &= ~static_cast<uint32_t>(layerB);
                        PhysicsOptions::collisionMatrix[indexB] &= ~static_cast<uint32_t>(layerA);
                    }
                }

                static bool CanCollide(LYNX_LAYER_SYSTEM layerA, LYNX_LAYER_SYSTEM layerB) {
                    if(layerA == LYNX_LAYER_SYSTEM::None || layerB == LYNX_LAYER_SYSTEM::None) return false;
                    
                    int indexA = GetBitIndex(static_cast<uint32_t>(layerA));
                    if (indexA < 0) return false;
                    
                    return (PhysicsOptions::collisionMatrix[indexA] & static_cast<uint32_t>(layerB)) != 0;
                }
    };
}*/