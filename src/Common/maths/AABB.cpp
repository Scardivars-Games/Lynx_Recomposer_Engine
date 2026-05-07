#include <ilen/maths/AABB.h>
#include <cmath>

namespace ilen::maths::aabb {

    //Como calcular esto: por cada eje se calcula la distancia minima para No Tocarse, se calcula la distancia entre los centros
    // y por ultimo se ve cuanto overlap hay.
    IntersectResult2D intersectAABB2D(const AABB2D& a, const AABB2D&b){
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

    IntersectResult3D intersectAABB3D(const AABB3D& a, const AABB3D&b){
        //Vector de a hacia b, infica la distancia y la Dirección.
        vec3 diff = b.center - a.center;

        //Determina cuanto se superponen las aabb (se suman las medias distancis, y luego, se resta la distancia entre los centros de los bloques.)
        float overlapX = (a.halfExtents.x + b.halfExtents.x) - std::abs(diff.x);
        float overlapY = (a.halfExtents.y + b.halfExtents.y) - std::abs(diff.y);
        float overlapZ = (a.halfExtents.z + b.halfExtents.z) - std::abs(diff.z);


        //Si ningun eje colisiona, descarte rápido
        if(overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f){
            return {.hit = false };
        }

        IntersectResult3D result;
        result.hit = true;

        if(overlapX <= overlapY && overlapX <= overlapZ){
            //El eje X esta más cerca del borde
            result.depth = overlapX;

            //el diff apunta a b. Si es mayor a 0, b esta a la derecha, y sino, b esta a la izquierda
            result.normal = vec3(diff.x < 0.0f? 1.0f :-1.0f, 0.0f, 0.0f);
        }else if(overlapY <= overlapX && overlapY <= overlapZ){
            result.depth = overlapY;
            result.normal = vec3(0.0f, diff.y < 0.0f ? 1.0f : -1.0f, 0.0f);
        }else{
            result.depth = overlapZ;
            result.normal = vec3(0.0f, 0.0f, diff.z < 0.0f ? 1.0f : -1.0f);
        }
        return result;
    }

    bool containsPoint2D(const AABB2D& box, vec2 point){
        vec2 diff = point - box.center;
        return std::abs(diff.x) <= box.halfExtents.x && std::abs(diff.y) <= box.halfExtents.y;
    }

    bool containsPoint3D(const AABB3D& box, vec3 point){
        vec3 diff = point - box.center;
        return std::abs(diff.x) <= box.halfExtents.x &&
            std::abs(diff.y) <= box.halfExtents.y &&
            std::abs(diff.z) <= box.halfExtents.z;
    }

    //Overlap, deteccion sencilla de solapamiento de cajas
    bool overlapAABB2D(const AABB2D& a, const AABB2D& b){
        vec2 diff = b.center - a.center;
        vec2 sum = a.halfExtents + b.halfExtents;

        return std::abs(diff.x) < sum.x && std::abs(diff.y) < sum.y;
    }

    bool overlapAABB3D(const AABB3D& a, const AABB3D& b){
        vec3 diff = b.center - a.center;
        vec3 sum = a.halfExtents + b.halfExtents;

        return std::abs(diff.x) < sum.x 
            && std::abs(diff.y) < sum.y 
            && std::abs(diff.z) < sum.z;
    }
}  