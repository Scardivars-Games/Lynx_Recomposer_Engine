#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

#include <ilen/physics/collider_validation_2d.h>

namespace {
    using ilen::maths::vector::vec2;
    using ilen::physics::IsValidConvexPolygon2D;

    bool expect(bool condition, std::string_view description) {
        if(!condition) {
            std::cerr << "[ERROR] " << description << '\n';
        }
        return condition;
    }
}

int main() {
    bool success = true;

    const std::vector<vec2> square{
        vec2{-1.0f, -1.0f},
        vec2{ 1.0f, -1.0f},
        vec2{ 1.0f,  1.0f},
        vec2{-1.0f,  1.0f}
    };

    success &= expect(
        IsValidConvexPolygon2D(square),
        "un cuadrado antihorario debe ser valido"
    );

    success &= expect(
        !IsValidConvexPolygon2D(
            std::vector<vec2>{
                vec2{-1.0f, -1.0f},
                vec2{-1.0f,  1.0f},
                vec2{ 1.0f,  1.0f},
                vec2{ 1.0f, -1.0f}
            }
        ),
        "un winding horario debe rechazarse"
    );

    success &= expect(
        !IsValidConvexPolygon2D(
            std::vector<vec2>{
                vec2{-1.0f, -1.0f},
                vec2{ 1.0f, -1.0f},
                vec2{ 1.0f, -1.0f},
                vec2{-1.0f,  1.0f}
            }
        ),
        "los vertices duplicados deben rechazarse"
    );

    success &= expect(
        !IsValidConvexPolygon2D(
            std::vector<vec2>{
                vec2{-1.0f, -1.0f},
                vec2{ 0.0f, -1.0f},
                vec2{ 1.0f, -1.0f},
                vec2{ 1.0f,  1.0f},
                vec2{-1.0f,  1.0f}
            }
        ),
        "los vertices colineales redundantes deben rechazarse"
    );

    auto nonFinite = square;
    nonFinite[2].x = std::numeric_limits<float>::quiet_NaN();
    success &= expect(
        !IsValidConvexPolygon2D(nonFinite),
        "los valores no finitos deben rechazarse"
    );

    std::vector<vec2> excessiveVertices;
    excessiveVertices.reserve(65);
    for(size_t index = 0; index < 65; ++index) {
        const float angle =
            static_cast<float>(index) * 2.0f * static_cast<float>(M_PI) / 65.0f;
        excessiveVertices.push_back(vec2{std::cos(angle), std::sin(angle)});
    }

    success &= expect(
        !IsValidConvexPolygon2D(excessiveVertices),
        "el limite de vertices debe aplicarse al registrar el collider"
    );

    if(success) {
        std::cout << "ColliderProxyBuilder2D: todas las pruebas pasaron.\n";
    }

    return success ? 0 : 1;
}
