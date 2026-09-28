#pragma once

#include <ilen/ecs/types.h>
#include <ilen/headers/maths.h>
#include <vector>

namespace ilen::ecs::components::physics {
    using vec2 =  maths::vector::vec2;

    //Almacena datos básicos de una única colision. NO ES COMPONENTE.

    //OLD
    /*struct CollisionContact2D {
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
    };*/

    //NEW
    struct ContactManifold2D {
        Entity entityA = ilen::ecs::NULL_ENTITY; //El mismo
        Entity entityB = ilen::ecs::NULL_ENTITY; //Otra entidad

        vec2 normal{0.0f}; //Direccion para que A salga de B
        vec2 point{0.0f}; //Punto de contacto en world space

        float penetration = 0.0f; //Penetracion del contacto (antes depth)
        float timeOfImpact = 1.0; //Instance del impacto intervalo [0, 1]
        bool hasContactPoint = false; //True si el punto de contacto fue calculado por un algoritmo de forma exacta
        bool isTrigger = false;
    };

    //Este componente es el que se añade a la entidad, es la susceptibles a los sistemas.
    struct CollisionComponent2D {
        std::vector<ContactManifold2D> contacts; //Un objeto puede tener multiples colision a la vez.
    };

    /*struct CollisionComponent3D {
        std::vector<CollisionContact3D> contacts; //Un objeto puede tener multiples colision a la vez.
    };*/
}