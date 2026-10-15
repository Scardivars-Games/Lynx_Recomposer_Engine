#pragma once

#include <lynx/headers/ecs.h>
#include <lynx/headers/maths.h>

namespace lynx::ecs::components::physics {
    using vec2 = maths::vector::vec2;
    using vec3 = maths::vector::vec3;

    LYNX_COMPONENT
    struct Rigidbody2D {
        //Velocidad del sistema
        LYNX_PROPERTY vec2 velocity{0.0f, 0.0f};
        
        //Que tanto afecta el valor por defecto de la gravedad al objeto
        LYNX_PROPERTY float gravityScale = 1.0f;

        //Velocidad máxima del objeto
        LYNX_PROPERTY float terminalVelocity = 50.0f;

        //Massa del objeto (para la física)
        LYNX_PROPERTY float mass = 1.0f;

        LYNX_PROPERTY vec2 prevPosition{0.0f}; // Esto es para calcular correctamente desde donde viene la colision, al chocar objetos dinamicos

        //Booleanos
        LYNX_PROPERTY bool isInGround = false;
        LYNX_PROPERTY bool usesGravity = true;
        LYNX_PROPERTY bool isStatic = false;

        // Un cuerpo cinemático se mueve por órdenes de posición, no recibe
        // gravedad ni fuerzas y no es desplazado por cuerpos dinámicos.
        // isStatic tiene prioridad si ambas opciones llegan a estar activas.
        LYNX_PROPERTY bool isKinematic = false;
    };

    //3D
    LYNX_COMPONENT
    struct Rigidbody3D {
        //Velocidad del sistema
        LYNX_PROPERTY vec3 velocity{0.0f};
        
        //Que tanto afecta el valor por defecto de la gravedad al objeto
        LYNX_PROPERTY float gravityScale = 1.0f;

        //Velocidad máxima del objeto
        LYNX_PROPERTY float terminalVelocity = 20.0f;

        //Booleanos
        LYNX_PROPERTY  bool isInGround = false;
        LYNX_PROPERTY bool usesGravity = true;
        LYNX_PROPERTY bool isStatic = false;
    };
}
