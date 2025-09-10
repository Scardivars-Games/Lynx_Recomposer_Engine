#pragma once

#include <ilen/ecs/types.h>
#include <set>

namespace ilen::ecs {
    //Todos los sistemas del motor (render, físicas, entradas, etc) heradan esta clase.
    class System {
        public:
            //Las entidades que este sistema debe de procesar.
            std::set<Entity> entities;
    };
}