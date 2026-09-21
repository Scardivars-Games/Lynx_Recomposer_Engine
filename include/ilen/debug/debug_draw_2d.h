#pragma once

//Código para ofrecer soporte para renderizador de Debug en el motor y mostrar datos internos usados para cálculos

#include <ilen/graphics/color.h>
#include <ilen/headers/maths.h>

#include <algorithm>
#include <mutex>
#include <vector>

namespace ilen::debug {
    using namespace ilen::graphics::color;
    using namespace ilen::graphics;
    using namespace ilen::maths::vector;

    //Configuración de como debe mostrarse las formas de depuración, si solo los bordes, o el objeto completo
    enum class DebugShapeMode2D {
        Wireframe,
        Solid
    };

    //Que tipo de primitiva es el objeto
    enum class DebugPrimitiveType2D {
        Line,
        Box
    };

    struct DebugPrimitive2D {
        DebugPrimitiveType2D type = DebugPrimitiveType2D::Line;

        //Line: a = inicio, b = final
        //Box: a = center, b = halfExtents
        vec2 a{0.0f};
        vec2 b{0.0f};

        graphics::Color color{1.0f, 1.0f, 1.0f, 1.0f};
        DebugShapeMode2D mode = DebugShapeMode2D::Wireframe;

        float thickness = 0.025f;
        float z = 0.0f;

        //0: un frame
        //> 0: cantidad de segundos a mostrar la primitiva
        //< 0: Mantener viva hasta una llamada explicita de Clear();
        float lifetime = 0.0f;
    };

    class DebugDraw2D {
        public:
            static void Line(
                vec2 start,
                vec2 end,
                Color color = Color{1.0f, 0.0f, 0.0f, 1.0f},
                float thickness = 0.025f,
                float lifetime = 0.0f,
                float z = 0.0f
            ) {
#if ILEN_DEBUG
                Push({
                    .type = DebugPrimitiveType2D::Line,
                    .a = start,
                    .b = end,
                    .color = color,
                    .mode = DebugShapeMode2D::Wireframe,
                    .thickness = thickness,
                    .z = z,
                    .lifetime = lifetime
                });
#endif
            }

            static void Box(
                vec2 center,
                vec2 size,
                Color color = Color{0.0f, 1.0f, 0.0f, 1.0f},
                DebugShapeMode2D mode = DebugShapeMode2D::Wireframe,
                float thickness = 0.025f,
                float lifetime = 0.0f,
                float z = 0.0f
            ) {
#if ILEN_DEBUG
                Push({
                    .type = DebugPrimitiveType2D::Box,
                    .a = center,
                    .b = size * 0.5f,
                    .color = color,
                    .mode = mode,
                    .thickness = thickness,
                    .z = z,
                    .lifetime = lifetime
                });
#endif
            }

            static void Ray(
                vec2 origin,
                vec2 direction,
                float distance,
                Color color = Color{0.0f,0.0f,1.0f,1.0f},
                float thickness = 0.025f,
                float lifetime = 0.0f,
                float z = 0.0f
            ) {
#if ILEN_DEBUG
                const float lenght = std::sqrt(
                    direction.x * direction.x + direction.y * direction.y
                );

                if(lenght <= 0.00001f || distance <= 0.0f) {
                    return;
                }

                const vec2 normalized {
                    direction.x / lenght,
                    direction.y / lenght
                };

                const vec2 end = origin + normalized * distance;

                Line(origin, end, color, thickness, lifetime, z);

                //Calculo de la felcha del rayo
                const float headLenght = std::min(0.25f, distance * 0.2f);
                const vec2 perpendicular{-normalized.y, normalized.x};
                const vec2 headBase = end - normalized * headLenght;

                Line(end, headBase + perpendicular * (headLenght * 0.5f), color, thickness, lifetime, z);
                Line(end, headBase - perpendicular * (headLenght * 0.5f), color, thickness, lifetime, z);
#endif
            }

            static void Point(
                vec2 position,
                float size,
                Color color = Color{1.0f,1.0f,1.0f,1.0f},
                float lifetime = 0.0f,
                float z = 0.0f
            ) {
#if ILEN_DEBUG
                Box(
                    position,
                    vec2{size, size},
                    color,
                    DebugShapeMode2D::Solid,
                    0.0f,
                    lifetime,
                    z
                );
#endif
            }

            static void Clear() {
#if ILEN_DEBUG
                std::lock_guard lock(s_mutex);
                s_primitives.clear();
#endif
            }

#if ILEN_DEBUG
            static std::vector<DebugPrimitive2D> Consume(float deltaTime) {
                std::lock_guard lock(s_mutex);

                std::vector<DebugPrimitive2D> result = s_primitives;

                std::erase_if(
                    s_primitives,
                    [deltaTime](DebugPrimitive2D& primitive) {
                        if(primitive.lifetime < 0.0f) {
                            return false;
                        }

                        if(primitive.lifetime == 0.0f) {
                            return true;
                        }

                        primitive.lifetime -=deltaTime;
                        return primitive.lifetime <= 0.0f;
                    }
                );

                return result;
            }

#endif

        private:
#if ILEN_DEBUG
            static void Push(DebugPrimitive2D primitive) {
                std::lock_guard lock(s_mutex);
                s_primitives.push_back(std::move(primitive));
            }

            static inline std::mutex s_mutex;
            static inline std::vector<DebugPrimitive2D> s_primitives;
#endif
    };
}