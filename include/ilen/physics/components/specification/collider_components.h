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

    //Collider proxy (usado para los calculos en el sistema)
    struct ColliderProxy2D {
        Entity entity = ilen::ecs::NULL_ENTITY;

        ILEN_COLLIDER_SHAPE_TYPE_2D shape = ILEN_COLLIDER_SHAPE_TYPE_2D::Box;

        AABB2D bounds = AABB2D::fromCenterHalf(
            vec2{0.0f}, vec2{0.0f}
        );

        std::vector<vec2> worldVertices;
        vec2 previousCenter{0.0f};

        bool isStatic = false;
        bool isTrigger = false;

        uint32_t layerMask = 0;
    };
}