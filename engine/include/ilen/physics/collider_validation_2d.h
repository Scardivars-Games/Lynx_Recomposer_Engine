#pragma once

#include <cmath>
#include <vector>

#include <ilen/maths/vector.h>

namespace ilen::physics {
    /** Valida winding, convexidad y estabilidad numérica del polígono local. */
    inline bool IsValidConvexPolygon2D(
        const std::vector<ilen::maths::vector::vec2>& vertices
    ) {
        using ilen::maths::vector::vec2;

        constexpr size_t maximumVertices = 64;
        constexpr float epsilon = 0.00001f;

        if(
            vertices.size() < 3 ||
            vertices.size() > maximumVertices
        ) {
            return false;
        }

        for(size_t first = 0; first < vertices.size(); ++first) {
            if(
                !std::isfinite(vertices[first].x) ||
                !std::isfinite(vertices[first].y)
            ) {
                return false;
            }

            for(size_t second = first + 1; second < vertices.size(); ++second) {
                const vec2 difference = vertices[first] - vertices[second];
                if(
                    ilen::maths::vector::dot(difference, difference) <=
                    epsilon * epsilon
                ) {
                    return false;
                }
            }
        }

        float doubledArea = 0.0f;

        for(size_t index = 0; index < vertices.size(); ++index) {
            const vec2& a = vertices[index];
            const vec2& b = vertices[(index + 1) % vertices.size()];
            const vec2& c = vertices[(index + 2) % vertices.size()];

            const vec2 edgeAB = b - a;
            const vec2 edgeBC = c - b;
            const float cross =
                edgeAB.x * edgeBC.y - edgeAB.y * edgeBC.x;

            doubledArea += a.x * b.y - a.y * b.x;

            // Los puntos colineales redundantes producen ejes casi nulos y no
            // aportan ninguna cara al polígono convexo.
            if(cross <= epsilon) {
                return false;
            }
        }

        return doubledArea > epsilon;
    }
}
