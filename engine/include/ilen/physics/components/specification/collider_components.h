#pragma once

#include <ilen/headers/ecs.h>
#include <ilen/maths/vector.h>
#include <vector>
#include <cstdint>
#include <ilen/maths/AABB.h>
#include <ilen/headers/enumerations.h>

namespace ilen::ecs::components::physics {
    using vec2 = maths::vector::vec2;
    using vec3 = maths::vector::vec3;

    //BOX
    ILEN_COMPONENT
    struct BoxCollider2D {
        ILEN_PROPERTY vec2 halfExtents{0.5f, 0.5f};
        ILEN_PROPERTY vec2 offset{0.0f, 0.0f};
        ILEN_PROPERTY bool isTrigger = false; //No opone resistencia al objeto, pero determina un espacio físico.
    };

    ILEN_COMPONENT
    struct BoxCollider3D {
        ILEN_PROPERTY vec3 halfExtents{0.5f};
        ILEN_PROPERTY vec3 offset{0.0f};
        ILEN_PROPERTY bool isTrigger = false; //No opone resistencia al objeto, pero determina un espacio físico.
    };

    //Poligono convexo
    ILEN_COMPONENT
    struct ConvexPolygonCollider2D {
        //Vertices en local space, sentido antihorario, formar polígono convexo, sin repetir el primer vertice al final, y al menos 3 puntos.
        ILEN_PROPERTY std::vector<vec2> vertices;
        ILEN_PROPERTY vec2 offset{0.0f, 0.0f};
        ILEN_PROPERTY bool isTrigger = false;
    };
}

namespace ilen::physics {
    using ilen::ecs::Entity;
    using ilen::maths::aabb::AABB2D;
    using ilen::maths::vector::vec2;
    using namespace ilen::enumerations;

    /**
     * Representación temporal de un collider dentro del pipeline de físicas.
     *
     * No es un componente ECS: se reconstruye para los cuerpos dinámicos y se
     * conserva para los estáticos. De esta forma los algoritmos de colisión no
     * necesitan consultar Transform ni los componentes de la entidad.
     */
    struct ColliderProxy2D {
        Entity entity = ilen::ecs::NULL_ENTITY;

        ILEN_COLLIDER_SHAPE_TYPE_2D shape = ILEN_COLLIDER_SHAPE_TYPE_2D::Box;

        AABB2D bounds = AABB2D::fromCenterHalf(
            vec2{0.0f}, vec2{0.0f}
        );

        // AABB que contiene tanto la posición actual como la posición prevista.
        // El broadphase lo usa para no perder objetos rápidos entre dos frames.
        AABB2D sweptBounds = AABB2D::fromCenterHalf(
            vec2{0.0f}, vec2{0.0f}
        );

        // Vértices convexos en world space en la posición actual del subpaso.
        std::vector<vec2> worldVertices;

        // Movimiento que se intentará realizar durante el tiempo restante del
        // subpaso. Swept SAT compara este movimiento contra el del otro proxy.
        vec2 stepTranslation{0.0f};

        // Velocidad usada exclusivamente por el solver durante este fixed
        // update. Incluye velocity y movimientos pedidos mediante Transform.
        vec2 motionVelocity{0.0f};
        vec2 commandedVelocity{0.0f};

        // Pivote y movimiento angular interno, expresado en radianes con la
        // misma convención usada para construir los vértices físicos.
        vec2 rotationPivot{0.0f};
        float maximumRadius = 0.0f;
        float stepRotation = 0.0f;
        float motionAngularVelocity = 0.0f;

        // Centro registrado al comenzar el fixed update; queda disponible para
        // depuración y futuras consultas de movimiento histórico.
        vec2 previousCenter{0.0f};

        bool isStatic = false;
        bool isKinematic = false;
        bool isTrigger = false;

        // Estado transitorio del solver. Un cuerpo encajado sigue participando
        // en colisiones, pero se comporta como inamovible durante este tick.
        bool solverFrozen = false;

        uint32_t layerMask = 0;
    };
}
