#pragma once

//Estructura para las cajas AABB del motor
#include <ilen/maths/vector.h>

namespace ilen::maths::aabb{
    using vec2 = ilen::maths::vector::vec2;
    using vec3 = ilen::maths::vector::vec3;

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
    IntersectResult2D intersectAABB2D(const AABB2D& a, const AABB2D&b);
    IntersectResult3D intersectAABB3D(const AABB3D& a, const AABB3D&b);

    //Funciones para comprobar si un punto existe dentro de un AABB
    bool containsPoint2D(const AABB2D& box, vec2 point);
    bool containsPoint3D(const AABB3D& box, vec3 point);

    //Version simplificada que comprueba superposiciones de AABB para operaciones rápidas
    bool overlapAABB2D(const AABB2D& a, const AABB2D&b);
    bool overlapAABB3D(const AABB3D& a, const AABB3D&b);
}