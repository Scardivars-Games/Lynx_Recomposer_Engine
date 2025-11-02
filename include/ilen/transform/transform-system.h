#pragma once

#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <ilen/headers/ecs-components.h>
#include <ilen/headers/maths.h>
#include <ilen/core/lifetime.h>

using namespace ilen::maths;

namespace ilen {
    struct EngineContext;
}

namespace ilen::ecs::systems {
    using namespace ilen::ecs::components;

    class TransformSystem : public ilen::ecs::System {
        public:
            void Init(ilen::EngineContext* engineContext);

            void Update();

        private:
            ilen::ecs::ECSCoordinator* p_Coordinator = nullptr;
            job_system::JobSystem* p_jobSystem = nullptr;

            void calculateLocalToWorldMatrix(Entity entity, matrix::mat4 parentMatrix);
    };
}