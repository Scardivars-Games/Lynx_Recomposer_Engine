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
    using ilen::ecs::components::physics::Containment2D;

    ColliderProxy2D makeBox(
        std::uint32_t entity,
        vec2 center,
        vec2 halfExtents,
        vec2 translation = vec2{0.0f},
        float rotation = 0.0f
    ) {
        ColliderProxy2D proxy;
        proxy.entity = entity;
        proxy.bounds = AABB2D::fromCenterHalf(center, halfExtents);
        proxy.sweptBounds = proxy.bounds;
        proxy.stepTranslation = translation;
        proxy.stepRotation = rotation;
        proxy.rotationPivot = center;
        proxy.maximumRadius = std::sqrt(
            halfExtents.x * halfExtents.x +
            halfExtents.y * halfExtents.y
        );
        proxy.worldVertices = {
            center + vec2{-halfExtents.x, -halfExtents.y},
            center + vec2{ halfExtents.x, -halfExtents.y},
            center + vec2{ halfExtents.x,  halfExtents.y},
            center + vec2{-halfExtents.x,  halfExtents.y}
        };
        return proxy;
    }

    ColliderProxy2D poseAtTime(
        const ColliderProxy2D& source,
        float time
    ) {
        ColliderProxy2D pose = source;
        const float radians = source.stepRotation * time;
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        const vec2 translation = source.stepTranslation * time;

        for(vec2& vertex : pose.worldVertices) {
            const vec2 local = vertex - source.rotationPivot;
            vertex = source.rotationPivot + translation + vec2{
                local.x * cosine - local.y * sine,
                local.x * sine + local.y * cosine
            };
        }

        pose.rotationPivot += translation;
        pose.stepTranslation = vec2{0.0f};
        pose.stepRotation = 0.0f;
        return pose;
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

    {
        auto rotatingBar = makeBox(
            15,
            vec2{0.0f, 0.0f},
            vec2{2.0f, 0.1f},
            vec2{0.0f},
            static_cast<float>(M_PI) * 0.5f
        );
        auto obstacle = makeBox(
            16,
            vec2{0.0f, 1.5f},
            vec2{0.15f}
        );
        const auto hit = NarrowPhase2D::Sweep(rotatingBar, obstacle);

        success &= expect(
            hit.has_value(),
            "una esquina que rota debe detectar un obstaculo intermedio"
        );
        success &= expect(
            hit && hit->timeOfImpact > 0.0f && hit->timeOfImpact < 1.0f,
            "el TOI angular debe quedar dentro del paso"
        );
        success &= expect(
            hit && hit->usedAngularSweep,
            "el manifold debe identificar el barrido angular"
        );
        success &= expect(
            hit && !NarrowPhase2D::Test(
                poseAtTime(rotatingBar, hit->timeOfImpact),
                obstacle
            ).has_value(),
            "el CCD angular debe dejar el cuerpo en la ultima pose libre"
        );
    }

    {
        auto rotatingBar = makeBox(
            17,
            vec2{0.0f, 0.0f},
            vec2{2.0f, 0.1f},
            vec2{0.0f},
            static_cast<float>(M_PI) * 0.5f
        );
        auto distantObstacle = makeBox(
            18,
            vec2{4.0f, 4.0f},
            vec2{0.1f}
        );

        success &= expect(
            !NarrowPhase2D::Sweep(rotatingBar, distantObstacle).has_value(),
            "una rotacion separada no debe generar falsos positivos"
        );
    }

    {
        auto movingAndRotating = makeBox(
            19,
            vec2{-3.0f, 0.0f},
            vec2{1.5f, 0.1f},
            vec2{4.0f, 0.0f},
            static_cast<float>(M_PI) * 0.5f
        );
        auto obstacle = makeBox(
            20,
            vec2{0.0f, 1.0f},
            vec2{0.2f}
        );

        success &= expect(
            NarrowPhase2D::Sweep(movingAndRotating, obstacle).has_value(),
            "traslacion y rotacion simultaneas deben compartir el mismo TOI"
        );
    }

    {
        auto rotatingIntoContact = makeBox(
            21,
            vec2{0.0f, 0.0f},
            vec2{1.0f, 0.1f},
            vec2{0.0f},
            static_cast<float>(M_PI) * 0.25f
        );
        auto obstacle = makeBox(
            22,
            vec2{0.8f, 0.2f},
            vec2{0.1f}
        );
        const auto hit = NarrowPhase2D::Sweep(
            rotatingIntoContact,
            obstacle
        );

        success &= expect(
            hit && hit->timeOfImpact <= 0.001f,
            "una rotacion que entra desde contacto debe impactar en TOI cero"
        );
    }

    {
        auto rotatingAway = makeBox(
            23,
            vec2{0.0f, 0.0f},
            vec2{1.0f, 0.1f},
            vec2{0.0f},
            static_cast<float>(M_PI) * -0.25f
        );
        auto obstacle = makeBox(
            24,
            vec2{0.8f, 0.2f},
            vec2{0.1f}
        );

        success &= expect(
            !NarrowPhase2D::Sweep(rotatingAway, obstacle).has_value(),
            "una rotacion que se aleja del contacto no debe bloquearse"
        );
    }

    {
        auto fullTurn = makeBox(
            25,
            vec2{0.0f, 0.0f},
            vec2{2.0f, 0.05f},
            vec2{0.0f},
            static_cast<float>(M_PI) * 2.0f
        );
        auto obstacle = makeBox(
            26,
            vec2{0.0f, 1.5f},
            vec2{0.05f}
        );

        success &= expect(
            NarrowPhase2D::Sweep(fullTurn, obstacle).has_value(),
            "un giro completo debe detectar impactos aunque termine donde inicio"
        );
    }

    {
        auto touchingThenTurningBack = makeBox(
            27,
            vec2{0.0f, 0.0f},
            vec2{1.0f, 0.1f},
            vec2{0.0f},
            static_cast<float>(M_PI) * -2.0f
        );
        auto obstacle = makeBox(
            28,
            vec2{0.8f, 0.2f},
            vec2{0.1f}
        );

        const auto hit = NarrowPhase2D::Sweep(
            touchingThenTurningBack,
            obstacle
        );

        success &= expect(
            hit.has_value(),
            "un giro completo que primero se aleja debe detectar el impacto posterior"
        );
        success &= expect(
            hit && hit->timeOfImpact > 0.35f,
            "el impacto posterior no debe confundirse con el contacto inicial"
        );
    }

    {
        const auto body = makeBox(
            29,
            vec2{0.0f, 0.0f},
            vec2{0.5f}
        );
        const auto touchingWall = makeBox(
            30,
            vec2{1.0f, 0.0f},
            vec2{0.5f}
        );
        const auto separatedWall = makeBox(
            31,
            vec2{1.01f, 0.0f},
            vec2{0.5f}
        );

        success &= expect(
            NarrowPhase2D::AreTouching(body, touchingWall, 0.001f),
            "el contacto exacto debe mantener bloqueada la rotacion"
        );
        success &= expect(
            !NarrowPhase2D::AreTouching(body, separatedWall, 0.001f),
            "la rotacion debe liberarse al separarse de la pared"
        );
    }

    {
        auto inner = makeBox(
            32,
            vec2{0.25f, 0.0f},
            vec2{0.5f}
        );
        auto solid = makeBox(
            33,
            vec2{0.0f, 0.0f},
            vec2{2.0f}
        );
        const auto contact = NarrowPhase2D::Test(inner, solid);

        success &= expect(
            contact && contact->containment == Containment2D::AInsideB,
            "SAT debe reconocer que A esta dentro del volumen solido de B"
        );

        if(contact) {
            for(vec2& vertex : inner.worldVertices) {
                vertex += contact->normal * contact->penetration;
            }

            success &= expect(
                !NarrowPhase2D::Test(inner, solid).has_value(),
                "el MTV de contencion debe expulsar A completamente del solido"
            );
        }
    }

    {
        auto solid = makeBox(
            34,
            vec2{0.0f, 0.0f},
            vec2{2.0f}
        );
        auto inner = makeBox(
            35,
            vec2{-0.25f, 0.0f},
            vec2{0.5f}
        );
        const auto contact = NarrowPhase2D::Test(solid, inner);

        success &= expect(
            contact && contact->containment == Containment2D::BInsideA,
            "SAT debe reconocer que B esta dentro del volumen solido de A"
        );

        if(contact) {
            for(vec2& vertex : inner.worldVertices) {
                vertex -= contact->normal * contact->penetration;
            }

            success &= expect(
                !NarrowPhase2D::Test(solid, inner).has_value(),
                "el MTV espejado debe expulsar B completamente del solido"
            );
        }
    }

    {
        const auto a = makeBox(
            36,
            vec2{0.0f, 0.0f},
            vec2{1.0f}
        );
        const auto b = makeBox(
            37,
            vec2{1.5f, 0.0f},
            vec2{1.0f}
        );
        const auto contact = NarrowPhase2D::Test(a, b);

        success &= expect(
            contact && contact->containment == Containment2D::None,
            "un solapamiento parcial no debe marcarse como contencion"
        );
    }

    {
        auto rotatedBody = makeBox(
            38,
            vec2{0.8f, 0.0f},
            vec2{1.0f},
            vec2{0.0f},
            static_cast<float>(M_PI) * 0.25f
        );
        rotatedBody = poseAtTime(rotatedBody, 1.0f);

        const auto wall = makeBox(
            39,
            vec2{2.0f, 0.0f},
            vec2{0.5f, 3.0f}
        );
        const auto contact = NarrowPhase2D::Test(rotatedBody, wall);

        success &= expect(
            contact && contact->normal.x < -0.99f,
            "un eje minimo de B debe expulsar A fuera de la pared, no hacia dentro"
        );

        const auto mirroredContact = NarrowPhase2D::Test(wall, rotatedBody);
        success &= expect(
            mirroredContact && mirroredContact->normal.x > 0.99f,
            "el manifold espejado debe invertir la normal del cuerpo rotado"
        );

        if(contact) {
            for(vec2& vertex : rotatedBody.worldVertices) {
                vertex += contact->normal * contact->penetration;
            }

            success &= expect(
                !NarrowPhase2D::Test(rotatedBody, wall).has_value(),
                "el MTV de un cuerpo rotado debe eliminar la penetracion"
            );
        }
    }

    if(success) {
        std::cout << "Swept SAT 2D: todas las pruebas pasaron.\n";
    }

    return success ? 0 : 1;
}
