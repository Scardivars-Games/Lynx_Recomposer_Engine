//Entity-Component system, tipos básicos de cada entidad.
#pragma once
#include <cstdint>
#include <bitset>

namespace ilen::ecs {
    //Es el ID que tiene la entidad en el motor.
    using Entity = std::uint32_t;
    const Entity MAX_ENTITIES = 10000;//Máximo 10 mil entidades a la vez.

    //Es un ID único para cada tipo de componente
    using ComponentType = std::uint8_t;
    const ComponentType MAX_COMPONENTS = 255;

    //Un bit flag que representa que componentes tiene la entidad.
    using Signature = std::bitset<MAX_COMPONENTS>;
} // namespace ilen::ecs
