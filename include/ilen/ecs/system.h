#pragma once

#include <ilen/ecs/types.h>
#include <set>

namespace ilen{
    struct EngineContext;
}

namespace ilen::ecs {
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
    };
}