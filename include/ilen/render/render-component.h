#pragma once

#include <ilen/headers/ecs.h>

namespace ilen::ecs::components {
    
    struct RenderComponent {
        uint32_t meshID;
        uint32_t mainTexID;
        uint32_t materialID;    
    };
}