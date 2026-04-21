#pragma once

#include <ilen/headers/ecs.h>
#include <ilen/headers/maths.h>

namespace ilen::ecs::components::physics {
    struct VelocityComponent {
        //Que tanto afecta el valor por defecto de la gravedad al objeto
        vec3 linearVelocity{0.0f, 0.0f, 0.0f};
    };
}