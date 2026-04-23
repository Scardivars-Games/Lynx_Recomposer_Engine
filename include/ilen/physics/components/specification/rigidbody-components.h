#pragma once

#include <ilen/headers/ecs.h>
#include <ilen/headers/maths.h>

namespace ilen::ecs::components::physics {
    using vec2 = maths::vector::vec2;
    using vec3 = maths::vector::vec3;

    struct Rigidbody2D {
        //Velocidad del sistema
        vec2 velocity{0.0f, 0.0f};
        
        //Que tanto afecta el valor por defecto de la gravedad al objeto
        float gravityScale = 1.0f;

        //Velocidad máxima del objeto
        float terminalVelocity = 20.0f;

        //Booleanos
        bool isInGround = false;
        bool usesGravity = true;
        bool isStatic = false;
    };

    struct BoxCollider2D {
        vec2 halfExtents{0.5f, 0.5f};
        vec2 offset{0.0f, 0.0f};
        bool isTrigger = false; //No opone resistencia al objeto, pero determina un espacio físico.
    };

    //3D
    struct Rigidbody3D {
        //Velocidad del sistema
        vec3 velocity{0.0f};
        
        //Que tanto afecta el valor por defecto de la gravedad al objeto
        float gravityScale = 1.0f;

        //Velocidad máxima del objeto
        float terminalVelocity = 20.0f;

        //Booleanos
        bool isInGround = false;
        bool usesGravity = true;
        bool isStatic = false;
    };

    struct BoxCollider3D {
        vec3 halfExtents{0.5f};
        vec3 offset{0.0f};
        bool isTrigger = false; //No opone resistencia al objeto, pero determina un espacio físico.
    };
}