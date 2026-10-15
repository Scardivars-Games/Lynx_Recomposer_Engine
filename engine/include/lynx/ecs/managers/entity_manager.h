#pragma once

#include <lynx/ecs/types.h>
#include <queue>
#include <vector>
#include <set>

namespace lynx::ecs {
    class EntityManager {
        public:
            explicit EntityManager(Entity maxEntities);

            //Crea una entidad y devuelve su id
            Entity createEntity();
            void destroyEntity(Entity entity); //Destruye una entidad para liberar su id

            void setSignature(Entity entity, Signature signature);  //Asigna una firma de componente a una entidad.

            Signature getSignature(Entity entity); //Obtiene la firma de componentes de una entidad.

            Entity getMaxEntities() const {
                return m_maxEntities;
            }

#ifdef LYNX_DEBUG
            const std::set<Entity>& getLiveEntities() const { return m_liveEntities; }
#endif
        private:
            void validateEntity(Entity entity) const;
            
            Entity m_maxEntities;
            //Cola de ID disponibles luego de la destrucción de alguna entidad
            std::queue<Entity> m_avalaiblesEntities {};
            //Arreglo de firmas, una para cada posible entidad
            std::vector<Signature> m_signatures{};
            //Número total de entidades que aún siguen vivas.
            uint32_t m_livingEntitiesCount = 0;
#ifdef LYNX_DEBUG
            std::set<Entity> m_liveEntities;
#endif
    };
}