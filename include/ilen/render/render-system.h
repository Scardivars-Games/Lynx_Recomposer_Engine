#pragma once

#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>

extern std::unique_ptr<ilen::ecs::ECSCoordinator> gCoordinator;

namespace ilen::ecs::systems {
    class RenderSystem : public ilen::ecs::System {

    };
}