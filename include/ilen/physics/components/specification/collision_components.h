#pragma once

#include <ilen/ecs/types.h>
#include <ilen/headers/maths.h>
#include <vector>

namespace ilen::ecs::components::physics {
    using vec2 =  maths::vector::vec2;

    //Almacena datos básicos de una única colision. NO ES COMPONENTE.
    struct CollisionContact2D {
        Entity other = NULL_ENTITY; //La otra entidad con que colisiono.
        vec2 normal{0.0f}; //Dirección del impacto
        float depth = 0.0f; //Indica cuan profundo es el impacto
        bool wasTrigger = false; //La posicion fue o no corregida, segun si es trigger o no.
    };

    struct CollisionContact3D {
        Entity other = NULL_ENTITY; //La otra entidad con que colisiono.
        vec3 normal{0.0f}; //Dirección del impacto
        float depth = 0.0f; //Indica cuan profundo es el impacto
        bool wasTrigger = false; //La posicion fue o no corregida, segun si es trigger o no.
    };


    //Este componente es el que se añade a la entidad, es la susceptibles a los sistemas.
    struct CollisionComponent2D {
        std::vector<CollisionContact2D> contacts; //Un objeto puede tener multiples colision a la vez.
    };

    struct CollisionComponent3D {
        std::vector<CollisionContact3D> contacts; //Un objeto puede tener multiples colision a la vez.
    };
}