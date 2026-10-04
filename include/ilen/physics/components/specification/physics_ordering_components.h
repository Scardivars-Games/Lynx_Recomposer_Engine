#pragma once

#include <ilen/ecs/types.h>
#include <ilen/headers/maths.h>

using namespace ilen::maths::vector;

namespace ilen::ecs::components::physics {
    struct PhysicsOrder {
        vec3 accumulatedForce {0.0f};
        vec3 accumulatedImpulse {0.0f};

        // Una velocidad solicitada se aplica al comenzar el siguiente fixed
        // update. Los ejes se controlan por separado para poder mover un
        // personaje horizontalmente sin borrar su velocidad vertical.
        vec2 velocityOverride {0.0f};
        bool overrideVelocityX = false;
        bool overrideVelocityY = false;

        // Movimiento transitorio consumido una sola vez por el siguiente paso
        // físico. accumulatedTranslation se barre; targetPosition puede ser
        // MovePosition (barrido) o Teleport (sólo contactos en el destino).
        vec2 accumulatedTranslation {0.0f};
        vec2 targetPosition {0.0f};
        bool hasMovePosition = false;
        bool hasTeleport = false;

        // Rotación 2D en grados visuales. Rotate/RotateTo atraviesan el CCD;
        // TeleportRotation aplica el ángulo directamente y sólo comprueba el
        // estado final, igual que Teleport para la posición.
        float accumulatedRotation = 0.0f;
        float targetRotation = 0.0f;
        bool hasRotateTo = false;
        bool hasTeleportRotation = false;

        bool hasOrders = false;
    };
}
