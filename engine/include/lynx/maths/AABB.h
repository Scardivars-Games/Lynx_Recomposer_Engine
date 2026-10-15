#pragma once

//Estructura para las cajas AABB del motor
#include <lynx/maths/vector.h>

namespace lynx::maths::aabb{
    using vec2 = lynx::maths::vector::vec2;
    using vec3 = lynx::maths::vector::vec3;

    enum class CollisionResolutionType {
        Static,
        DynamicVsStatic,
        DynamicVsDynamic
    };

    //Caja AABB para el 2D
    struct AABB2D {
        vec2 center; //centro del AABB en espacio GLOBAL
        vec2 halfExtents; //Mitad del tamaño de la caja por cada eje.
        
        //Constructores
        static AABB2D fromCenterHalf(vec2 center, vec2 half){
            return {center, half};
        }

        //Constructor a partir de las esquinas min-max del objeto.
        static AABB2D fromMinMax(vec2 min, vec2 max){
            vec2 half = (max - min) * 0.5f;
            vec2 center = min + half;
            return {center, half};
        }

        //Helpers geometricos
        vec2 min() const { return center - halfExtents; }
        vec2 max() const { return center + halfExtents; }

        //Utilidades
        //Expandir un AABB para contener un punto nuevo.
        void expandToContain(vec2 point){
            *this = fromMinMax(vector::min(min(), point), vector::max(max(), point));
        }

        //Retorna una caja que envuelve la actual AABB y a ostream
        AABB2D merge(const AABB2D& other){
            return fromMinMax(vector::min(min(), other.min()), vector::max(max(), other.max()));
        }
    };

    //AABB para elementos 3D
    struct AABB3D{
        vec3 center;
        vec3 halfExtents;

        //Constructores (AABB2D explica cada uno)
        static AABB3D fromCenterHalf(vec3 center, vec3 half){
            return {center, half};
        }

        static AABB3D fromMinMax(vec3 minCorner, vec3 maxCorner){
            vec3 half = (maxCorner - minCorner) * 0.05f;
            vec3 center = minCorner + half;

            return {center, half};
        }

        vec3 min() const { return center - halfExtents; }
        vec3 max() const { return center + halfExtents; }

        //Métodos (la version 2D explica que hace cada uno)
        void expandToContain(vec3 point){
            *this = fromMinMax(vector::min(min(), point), vector::max(max(), point));
        }

        //Retorna una caja que envuelve la actual AABB y a ostream
        AABB3D merge(const AABB3D& other){
            return fromMinMax(vector::min(min(), other.min()), vector::max(max(), other.max()));
        }
    };

    //Estructura que ofrece datos cuando intersectan 2 AABB.
    struct IntersectResult2D{
        bool hit = false;
        vec2 normal = vec2(0.0f);
        float depth = 0.0f;
    };

    struct IntersectResult3D{
        bool hit = false;
        vec3 normal = vec3(0.0f);
        float depth = 0.0f;
    };

    //Funciones con respecto a las intersecciones
    template<CollisionResolutionType type>
    IntersectResult2D intersectAABB2D(const AABB2D& a, const AABB2D&b, const vec2& a_prevCenter = vec2(0.0f), const vec2& b_prevCenter = vec2(0.0f)) {
        //Vector de a hacia b, infica la distancia y la Dirección.
        vec2 diff = b.center - a.center;

        //Determina cuanto se superponen las aabb (se suman las medias distancis, y luego, se resta la distancia entre los centros de los bloques.)
        float overlapX = (a.halfExtents.x + b.halfExtents.x) - std::abs(diff.x);
        float overlapY = (a.halfExtents.y + b.halfExtents.y) - std::abs(diff.y);

        //Si ningun eje colisiona, descarte rápido
        if(overlapX <= 0.0f || overlapY <= 0.0f){
            return {.hit = false };
        }

        IntersectResult2D result;
        result.hit = true;
        
        if constexpr (type == CollisionResolutionType::Static){
            if(overlapX < overlapY){
                //El eje X esta más cerca del borde
                result.depth = overlapX;

                //el diff apunta a b. Si es mayor a 0, b esta a la derecha, y sino, b esta a la izquierda
                result.normal = vec2(diff.x < 0.0f ? 1.0f : -1.0f, 0.0f);
            }else{
                result.depth = overlapY;
                result.normal = vec2(0.0f, diff.y < 0.0f ? 1.0f : -1.0f);
            }
            return result;
        } 
        //Dynamiv Vs Static
        else if constexpr (type == CollisionResolutionType::DynamicVsStatic){
            constexpr float epsilon = 0.001f;

            const float previousLeft = a_prevCenter.x - a.halfExtents.x;
            const float previousRight = a_prevCenter.x + a.halfExtents.x;
            const float previousBottom = a_prevCenter.y - a.halfExtents.y;
            const float previousTop = a_prevCenter.y + a.halfExtents.y;

            const float blockLeft = b.center.x - b.halfExtents.x;
            const float blockRight = b.center.x + b.halfExtents.x;
            const float blockBottom = b.center.y - b.halfExtents.y;
            const float blockTop = b.center.y + b.halfExtents.y;

            const vec2 movement = a.center - a_prevCenter;

            const bool fromAbove = previousBottom >= blockTop - epsilon && movement.y <= 0.0f;
            const bool fromBelow = previousTop <= blockBottom + epsilon && movement.y >= 0.0f;
            const bool fromLeft = previousRight <= blockLeft + epsilon && movement.x >= 0.0f;
            const bool fromRight = previousLeft >= blockRight - epsilon && movement.x <= 0.0f;

            const bool verticalEntry = fromAbove || fromBelow;
            const bool horizontalEntry = fromLeft || fromRight;

            bool resolveY;

            if(verticalEntry && horizontalEntry){
                //Entrada diagonal
                const float entryX = std::abs(movement.x) > epsilon
                ? (fromLeft 
                    ? (blockLeft - previousRight) / movement.x
                    : (blockRight - previousLeft) / movement.x)
                : 0.0f;

                const float entryY = std::abs(movement.y) > epsilon
                ? (fromBelow
                    ? (blockBottom - previousTop) / movement.y
                    : (blockTop - previousBottom) / movement.y)
                : 0.0f;

                resolveY = entryY >= entryX;
            }else if(verticalEntry) {
                resolveY = true;
            }else if(horizontalEntry) {
                resolveY = false;
            } else{
                //Esto indica que existia penetración, recuperar la anterior para terminar de resolver
                resolveY = overlapY <= overlapX;
            }

            if(resolveY){
                const bool pushUp = verticalEntry ? fromAbove : diff.y < 0.0f;

                result.normal = vec2(0.0f, pushUp ? 1.0f : -1.0f);

                // Distancia hasta la cara elegida. No depender del centro
                // actual para decidir de qué lado debe salir el jugador.
                result.depth = pushUp 
                    ? blockTop - (a.center.y - a.halfExtents.y) 
                    : (a.center.y + a.halfExtents.y) - blockBottom;
            }else{
                const bool pushRight = horizontalEntry ? fromRight : diff.x < 0.0f;
                
                result.normal = vec2(pushRight ? 1.0f : -1.0f, 0.0f);
                result.depth = pushRight
                    ? blockRight - (a.center.x - a.halfExtents.x)
                    : (a.center.x + a.halfExtents.x) - blockLeft;
            }
        }
        //Dynamic VS Dynamic
        else if constexpr(type == CollisionResolutionType::DynamicVsDynamic){
            float a_prevBottom = a_prevCenter.y - a.halfExtents.y;
            float a_prevTop = a_prevCenter.y + a.halfExtents.y;
            float b_prevTop = b_prevCenter.y + b.halfExtents.y;
            float b_prevBottom = b_prevCenter.y - b.halfExtents.y;

            if(a_prevBottom >= b_prevTop) {
                result.depth = overlapY;
                result.normal = vec2(0.0f, 1.0f);
            }else if(a_prevTop <= b_prevBottom) {
                result.depth = overlapY;
                result.normal = vec2(0.0f, -1.0f);
            }else {
                result.depth = overlapX;
                result.normal = vec2(diff.x < 0.0f ? 1.0f : -1.0f, 0.0f);
            }
        }

        return result;
    }


    IntersectResult3D intersectAABB3D(const AABB3D& a, const AABB3D&b);

    //Funciones para comprobar si un punto existe dentro de un AABB
    bool containsPoint2D(const AABB2D& box, vec2 point);
    bool containsPoint3D(const AABB3D& box, vec3 point);

    //Version simplificada que comprueba superposiciones de AABB para operaciones rápidas
    bool overlapAABB2D(const AABB2D& a, const AABB2D&b);
    bool overlapAABB3D(const AABB3D& a, const AABB3D&b);
}