#pragma once
#include <cstdint>
#include <type_traits>
#include <ilen/headers/enumerations.h>
#include <ilen/core/runtime_options.h>

namespace ilen::physics {
    using namespace ilen::enumerations;
    using namespace ilen::core::options;
    
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

                static void SetLayerCollision(ILEN_LAYER_SYSTEM layerA, ILEN_LAYER_SYSTEM layerB, bool canCollide){
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

                static bool CanCollide(ILEN_LAYER_SYSTEM layerA, ILEN_LAYER_SYSTEM layerB) {
                    if(layerA == ILEN_LAYER_SYSTEM::None || layerB == ILEN_LAYER_SYSTEM::None) return false;
                    
                    int indexA = GetBitIndex(static_cast<uint32_t>(layerA));
                    if (indexA < 0) return false;
                    
                    return (PhysicsOptions::collisionMatrix[indexA] & static_cast<uint32_t>(layerB)) != 0;
                }
    };
}