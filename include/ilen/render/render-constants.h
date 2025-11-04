#pragma once

#include <cstdint>

namespace ilen::render {
    //En este archivo se definen todas las convenciones del motor de renderizado de nuestro engine. Esto incluye los slots de los buffers por defecto del motor
    constexpr uint32_t RenderBufferSlot_VertexMain = 0;
    constexpr uint32_t RenderBufferSlot_InstanceModel = 1;
    //Buffers 2,3 y 4 se dejan a disposición libre del usuario. Principalmente deben ser usados para datos de vertices y matrices de un objeto, no el ámbito global

    constexpr uint32_t GlobalBufferSlot_Base = 5; //Usado para calcular el slot
    constexpr uint32_t GlobalBufferSlot_Camera = GlobalBufferSlot_Base + 0; //Slot 5
    constexpr uint32_t GlobalBufferSlot_Lights = GlobalBufferSlot_Base + 1; //Slot 6

    constexpr uint32_t MaterialBufferSlot = 15; //Uniforms de x material

    //Texturas
    constexpr uint32_t TextureBufferSlot_Base = 0;
    constexpr uint32_t TextureBufferSlot_NormalMap = 1;
    constexpr uint32_t TextureBufferSlot_Metallic = 2;

    //Limites y reglas
    constexpr uint32_t UserBufferStart = 7;
    constexpr uint32_t UserBufferEnd = 14;
}