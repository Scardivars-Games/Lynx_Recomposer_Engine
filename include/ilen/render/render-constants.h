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

            //Texturas para PostProcesado (este mismo índice se usara para activar o no el dato en el UberShader de post_procesado)
            ILEN_CONST uint32_t TextureBufferSlot_Color = 0;
            ILEN_CONST uint32_t TextureBufferSlot_VelocityBuffer = 1;

            //Render Pass fixed indices
            ILEN_CONST uint32_t RenderPassAttachment_Color = 0;
            ILEN_CONST uint32_t RenderPassAttachment_VelocityBuffer = 1;

            //Limites y reglas
            ILEN_CONST uint32_t UserBufferStart = 7;
            ILEN_CONST uint32_t UserBufferEnd = 14;
        }

        namespace structs {
            //Unimos las camaras para no tener que crear 2 buffers con datos redundantes.
            struct CameraData {
                float4x4 currentViewProjectionMatrix;
                float4x4 previousViewProjectionMatrix;
            };

            struct InstanceData {
                float4x4 modelMatrix;
            };

            struct InstanceDataForVelocityBuffer {
                float4x4 actualModelMatrix;
                float4x4 previousModelMatrix;
            };

            //A partir de aqui se definen metadatos para ser consumidos por ciertas APIs. Metal no requiere esto, por lo que en macOS no se añade
            enum GlobalUniformIdentifier {
                CAMERA_DATA,
                INSTANCE_DATA,
                INSTANCE_DATA_FOR_VELOCITY_BUFFER,
                NOT_DEFINED, //Para forzar al usuario a definir este dato. Vital para el motor.
            };

            enum ILEN_VULKAN_DESCRIPTOR_TYPE {
                ILEN_VULKAN_DESCRIPTOR_UNIFORM_BUFFER,
                ILEN_VULKAN_DESCRIPTOR_STORAGE_BUFFER,
            };

            struct GlobalUniformsMetadata {
                GlobalUniformIdentifier identifier;
                uint32_t bindingSlot;
                uint32_t setSlot;
                size_t size;
                ILEN_VULKAN_DESCRIPTOR_TYPE type;
            };
#if !__METAL_VERSION__
            inline std::vector<GlobalUniformsMetadata> getGlobalShaderStructs() {
                return {
                        {CAMERA_DATA, slots::GlobalBufferSlot_Camera, 0, sizeof(CameraData), ILEN_VULKAN_DESCRIPTOR_UNIFORM_BUFFER},
                        {INSTANCE_DATA, slots::RenderBufferSlot_InstanceModel, 0, sizeof(InstanceData), ILEN_VULKAN_DESCRIPTOR_STORAGE_BUFFER},
                        {INSTANCE_DATA_FOR_VELOCITY_BUFFER, slots::RenderBufferSlot_InstanceModel, 0, sizeof(InstanceDataForVelocityBuffer), ILEN_VULKAN_DESCRIPTOR_STORAGE_BUFFER},
                    };
            }
#endif


            //Para ser usado en el calculo del velocity buffer para efectos de Post Procesado.
            /*struct VelocityCameraData {
                float4x4 currentViewProjectionMatrix;
                float4x4 previousViewProjectionMatrix;
            };*/
        }
    }
}
