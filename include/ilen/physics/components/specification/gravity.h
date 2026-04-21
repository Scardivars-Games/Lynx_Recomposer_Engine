#pragma once

#include <ilen/headers/ecs.h>

namespace ilen::ecs::components::physics {
    struct GravityComponent {
        //Que tanto afecta el valor por defecto de la gravedad al objeto
        float gravityScale = 1.0f;
    };
}