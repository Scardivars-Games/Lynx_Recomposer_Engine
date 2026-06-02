#pragma once

#include <ilen/ecs/types.h>
#include <ilen/headers/maths.h>

using namespace ilen::maths::vector;

namespace ilen::ecs::components::physics {
    struct PhysicsOrder {
        vec3 accumulatedForce {0.0f};
        vec3 accumulatedImpulse {0.0f};
        bool hasOrders = false;
    };
}