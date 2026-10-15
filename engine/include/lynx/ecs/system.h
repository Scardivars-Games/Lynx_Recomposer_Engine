#pragma once

#include <lynx/ecs/types.h>
#include <set>

namespace lynx{
    struct EngineContext;
}

namespace lynx::job_system {
    class CommandBufferDispatcher;
    class JobSystem;
}

namespace lynx::ecs {
    class ECSCoordinator;
}

namespace lynx::ecs {
    enum class SystemPhase {
        FixedUpdate, //Datos que deben calcularse cada cierta cantidad fija
        VariableUpdate, //Datos que se pueden calcular en el Update normal
        PreRender, //Sistemas que deben calcularse despues del Update, pero antes de renderizar (como la Cámara y sistemas que alimentan el render)
        Render //Sistemas que dibujan a la GPU.
    };
    
    //Todos los sistemas del motor (render, físicas, entradas, etc) heradan esta clase.
    class ISystem {
        public:
            //Las entidades que este sistema debe de procesar.
            std::set<Entity> entities;

            virtual ~ISystem() = default;

            virtual void Init(const EngineContext* engineContext) {};
            virtual void Update() {};
        protected:
            lynx::ecs::ECSCoordinator* p_coordinator = nullptr;
            lynx::job_system::JobSystem* p_jobSystem = nullptr;
            lynx::job_system::CommandBufferDispatcher* p_commandDispatcher = nullptr;
    };
}