#pragma once

#include <ilen/ecs/types.h>
#include <ilen/headers/ecs-components.h>

namespace ilen::engine::api{
    using namespace ilen::ecs;
    using namespace components;

    class Physics2DAPI {
        public:
            static void Init(const ilen::EngineContext* context);
            static void GenerateBoxColliderSize(Entity e, physics::BoxCollider2D* collider);
        
        private:
            static inline ilen::ecs::ECSCoordinator* p_coordinator = nullptr;    
    };
}