#pragma once

#include <ilen/headers/ecs.h>
#include <ilen/headers/maths.h>

namespace ilen::ecs::components::physics {
    using vec2 = maths::vector::vec2;
    using vec3 = maths::vector::vec3;

    ILEN_COMPONENT
    struct alignas(64) Rigidbody2D {
        //Velocidad del sistema
        ILEN_PROPERTY vec2 velocity{0.0f, 0.0f};
        
        //Que tanto afecta el valor por defecto de la gravedad al objeto
        ILEN_PROPERTY float gravityScale = 1.0f;

        //Velocidad máxima del objeto
        ILEN_PROPERTY float terminalVelocity = 50.0f;

        //Massa del objeto (para la física)
        ILEN_PROPERTY float mass = 1.0f;

        ILEN_PROPERTY vec2 prevPosition{0.0f}; // Esto es para calcular correctamente desde donde viene la colision, al chocar objetos dinamicos

        //Booleanos
        ILEN_PROPERTY bool isInGround = false;
        ILEN_PROPERTY bool usesGravity = true;
        ILEN_PROPERTY bool isStatic = false;
    };

    ILEN_COMPONENT
    struct BoxCollider2D {
        ILEN_PROPERTY vec2 halfExtents{0.5f, 0.5f};
        ILEN_PROPERTY vec2 offset{0.0f, 0.0f};
        ILEN_PROPERTY bool isTrigger = false; //No opone resistencia al objeto, pero determina un espacio físico.
    };

    //3D
    ILEN_COMPONENT
    struct alignas(64) Rigidbody3D {
        //Velocidad del sistema
        ILEN_PROPERTY vec3 velocity{0.0f};
        
        //Que tanto afecta el valor por defecto de la gravedad al objeto
        ILEN_PROPERTY float gravityScale = 1.0f;

        //Velocidad máxima del objeto
        ILEN_PROPERTY float terminalVelocity = 20.0f;

        //Booleanos
        ILEN_PROPERTY  bool isInGround = false;
        ILEN_PROPERTY bool usesGravity = true;
        ILEN_PROPERTY bool isStatic = false;
    };

    ILEN_COMPONENT
    struct BoxCollider3D {
        ILEN_PROPERTY vec3 halfExtents{0.5f};
        ILEN_PROPERTY vec3 offset{0.0f};
        ILEN_PROPERTY bool isTrigger = false; //No opone resistencia al objeto, pero determina un espacio físico.
    };
}