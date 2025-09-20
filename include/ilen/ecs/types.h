//Entity-Component system, tipos básicos de cada entidad.
#pragma once
#include <cstdint>
#include <bitset>
#include <limits>

namespace ilen::ecs {
    //Es el ID que tiene la entidad en el motor.
    using Entity = std::uint32_t;
    const Entity MAX_ENTITIES = 10000;//Máximo 10 mil entidades a la vez.
    const Entity NULL_ENTITY = std::numeric_limits<Entity>::max();

    //Es un ID único para cada tipo de componente
    using ComponentType = std::uint8_t;
    const ComponentType MAX_COMPONENTS = 254;
    const ComponentType NULL_COMPONENT = 255;

    //Un bit flag que representa que componentes tiene la entidad.
    using Signature = std::bitset<MAX_COMPONENTS>;
} // namespace ilen::ecs
