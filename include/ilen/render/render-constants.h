#pragma once

#ifdef __METAL_VERSION__
#include <metal_stdlib>
using namespace metal;
using float4x4 = metal::float4x4;
using float3 = metal::float3;
using float2 = metal::float2;
#define CONSTANT constant
#define ILEN_CONST constant
#else
#include <cstdint>
#include <ilen/headers/maths.h>
using float4x4 = ilen::maths::matrix::mat4;
#define CONSTANT
#define ILEN_CONST constexpr
#endif

namespace ilen {
    namespace render {
        namespace slots {
            //Atributos para el vertex buffer
            ILEN_CONST uint32_t VertexBufferAttribute_Position = 0;
            ILEN_CONST uint32_t VertexBufferAttribute_Color = 1;
            ILEN_CONST uint32_t VertexBufferAttribute_UV = 2;
            ILEN_CONST uint32_t VertexBufferAttribute_Normal = 3;

            //En este archivo se definen todas las convenciones del motor de renderizado de nuestro engine. Esto incluye los slots de los buffers por defecto del motor
            ILEN_CONST uint32_t RenderBufferSlot_VertexMain = 0;
            ILEN_CONST uint32_t RenderBufferSlot_InstanceModel = 1;
            //Buffers 2,3 y 4 se dejan a disposición libre del usuario. Principalmente deben ser usados para datos de vertices y matrices de un objeto, no el ámbito global

            ILEN_CONST uint32_t GlobalBufferSlot_Base = 5; //Usado para calcular el slot
            ILEN_CONST uint32_t GlobalBufferSlot_Camera = GlobalBufferSlot_Base + 0; //Slot 5
            ILEN_CONST uint32_t GlobalBufferSlot_Lights = GlobalBufferSlot_Base + 1; //Slot 6

            ILEN_CONST uint32_t MaterialBufferSlot = 15; //Uniforms de x material

            //Texturas
            ILEN_CONST uint32_t TextureBufferSlot_Base = 0;
            ILEN_CONST uint32_t TextureBufferSlot_NormalMap = 1;
            ILEN_CONST uint32_t TextureBufferSlot_Metallic = 2;

            //Limites y reglas
            ILEN_CONST uint32_t UserBufferStart = 7;
            ILEN_CONST uint32_t UserBufferEnd = 14;
        }

        namespace structs {
            struct CameraData {
                float4x4 viewProjectionMatrix;
            };

            struct InstanceData {
                float4x4 modelMatrix;
            };
        }
    }
}

