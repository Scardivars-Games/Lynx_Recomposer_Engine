#pragma once
#include <ilen/ecs/system.h>
#include <ilen/core/lifetime.h>

namespace  ilen::ecs::systems {
    class PostProcessSystem final: public System {
    public:
        void Init(const EngineContext * engineContext);
        void Update();
    private:
        ECSCoordinator* coordinator = nullptr;
        render::IRenderer* renderer = nullptr;

        const char* DEBUG_PREFIX = "POST_PROCESSING_SYSTEM";
    };
}
