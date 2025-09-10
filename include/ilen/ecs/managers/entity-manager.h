#pragma once

#include <ilen/ecs/types.h>
#include <queue>
#include <array>

namespace ilen::ecs {
    class EntityManager {
        public:
            EntityManager();

            //Crea una entidad y devuelve su id
            Entity createEntity();

            void destroyEntity(Entity entity); //Destruye una entidad para liberar su id

            void setSignature(Entity entity, Signature signature);  //Asigna una firma de componente a una entidad.

            Signature getSignature(Entity entity); //Obtiene la firma de componentes de una entidad.

        private:
            //Cola de ID disponibles luego de la destrucción de alguna entidad
            std::queue<Entity> avalaiblesEntities {};
            //Arreglo de firmas, una para cada posible entidad
            std::array<Signature, MAX_ENTITIES> signatures{};
            //Número total de entidades que aún siguen vivas.
            uint32_t livingEntitiesCount{};
    };
}