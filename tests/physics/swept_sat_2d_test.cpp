#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>

#include <ilen/physics/narrow_phase_2d.h>

namespace {
    using ilen::maths::aabb::AABB2D;
    using ilen::maths::vector::vec2;
    using ilen::physics::ColliderProxy2D;
    using ilen::physics::NarrowPhase2D;

    ColliderProxy2D makeBox(
        std::uint32_t entity,
        vec2 center,
        vec2 halfExtents,
        vec2 translation = vec2{0.0f}
    ) {
        ColliderProxy2D proxy;
        proxy.entity = entity;
        proxy.bounds = AABB2D::fromCenterHalf(center, halfExtents);
        proxy.sweptBounds = proxy.bounds;
        proxy.stepTranslation = translation;
        proxy.worldVertices = {
            center + vec2{-halfExtents.x, -halfExtents.y},
            center + vec2{ halfExtents.x, -halfExtents.y},
            center + vec2{ halfExtents.x,  halfExtents.y},
            center + vec2{-halfExtents.x,  halfExtents.y}
        };
        return proxy;
    }

    bool approximatelyEqual(float a, float b, float epsilon = 0.0001f) {
        return std::abs(a - b) <= epsilon;
    }

    bool expect(bool condition, std::string_view description) {
        if(!condition) {
            std::cerr << "[ERROR] " << description << '\n';
        }
        return condition;
    }
}

int main() {
    bool success = true;

    {
        auto moving = makeBox(1, vec2{0.0f, 0.0f}, vec2{0.5f}, vec2{10.0f, 0.0f});
        auto wall = makeBox(2, vec2{5.0f, 0.0f}, vec2{0.5f});
        const auto hit = NarrowPhase2D::Sweep(moving, wall);

        success &= expect(hit.has_value(), "un cuerpo rapido debe detectar la pared");
        success &= expect(
            hit && approximatelyEqual(hit->timeOfImpact, 0.4f),
            "el TOI contra la pared debe ser 0.4"
        );
        success &= expect(
            hit && hit->normal.x < -0.99f,
            "la normal debe sacar al cuerpo movil hacia la izquierda"
        );
    }

    {
        auto moving = makeBox(3, vec2{0.0f, 0.0f}, vec2{0.5f}, vec2{10.0f, 0.0f});
        auto separated = makeBox(4, vec2{5.0f, 3.0f}, vec2{0.5f});

        success &= expect(
            !NarrowPhase2D::Sweep(moving, separated).has_value(),
            "el movimiento paralelo separado no debe generar falsos positivos"
        );
    }

    {
        auto a = makeBox(5, vec2{0.0f, 0.0f}, vec2{0.5f}, vec2{4.0f, 0.0f});
        auto b = makeBox(6, vec2{6.0f, 0.0f}, vec2{0.5f}, vec2{-2.0f, 0.0f});
        const auto hit = NarrowPhase2D::Sweep(a, b);

        success &= expect(hit.has_value(), "dos cuerpos en movimiento deben usar velocidad relativa");
        success &= expect(
            hit && approximatelyEqual(hit->timeOfImpact, 5.0f / 6.0f),
            "el TOI dinámico-dinámico debe usar el recorrido relativo"
        );
    }

    {
        auto a = makeBox(7, vec2{0.0f, 0.0f}, vec2{1.0f}, vec2{1.0f, 0.0f});
        auto b = makeBox(8, vec2{0.5f, 0.0f}, vec2{1.0f});

        success &= expect(
            !NarrowPhase2D::Sweep(a, b).has_value(),
            "Sweep debe delegar solapamientos iniciales al SAT discreto"
        );
        success &= expect(
            NarrowPhase2D::Test(a, b).has_value(),
            "SAT discreto debe resolver el solapamiento inicial"
        );
    }

    {
        auto movingIntoWall = makeBox(
            9,
            vec2{0.0f, 0.0f},
            vec2{0.5f},
            vec2{1.0f, 0.0f}
        );
        auto wall = makeBox(10, vec2{1.0f, 0.0f}, vec2{0.5f});
        const auto hit = NarrowPhase2D::Sweep(movingIntoWall, wall);

        success &= expect(
            hit && approximatelyEqual(hit->timeOfImpact, 0.0f),
            "un contacto que intenta entrar debe activarse en TOI cero"
        );
    }

    {
        auto movingAway = makeBox(
            11,
            vec2{0.0f, 0.0f},
            vec2{0.5f},
            vec2{-1.0f, 0.0f}
        );
        auto wall = makeBox(12, vec2{1.0f, 0.0f}, vec2{0.5f});

        success &= expect(
            !NarrowPhase2D::Sweep(movingAway, wall).has_value(),
            "un contacto que se separa no debe bloquear el movimiento"
        );
    }

    {
        auto movingTangentially = makeBox(
            13,
            vec2{0.0f, 0.0f},
            vec2{0.5f},
            vec2{0.0f, 1.0f}
        );
        auto wall = makeBox(14, vec2{1.0f, 0.0f}, vec2{0.5f});

        success &= expect(
            !NarrowPhase2D::Sweep(movingTangentially, wall).has_value(),
            "un contacto tangencial no debe congelar el movimiento libre"
        );
    }

    if(success) {
        std::cout << "Swept SAT 2D: todas las pruebas pasaron.\n";
    }

    return success ? 0 : 1;
}
