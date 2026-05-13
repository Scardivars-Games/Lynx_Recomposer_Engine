#pragma once

#include <ilen/headers/ecs.h>
#include <ilen/game-logic/script-system.h>
#include <ilen/core/engine-context.h>

using namespace ilen::ecs;

namespace ilen::engine::api {
    //API diseñada para interactuar con las entidades del ECS. Esto lo conseguira mediante el MetadataComponent, por lo que lo modificara, lo analizara, y retornara entidades.
    class EntityAPI {
        friend class systems::ScriptingSystem;

        public:
            static void SetActive(Entity entity, bool newState);
            static bool isActive(Entity entity);
            static Entity GetEntityByName(std::string entityName);
        private:
            static void Init(const ilen::EngineContext* context);
            static void ResetMetadataPointer(); //Resetea el puntero de los metadatos, para obtener uno válido el próximo frame.
            static inline void GetMetadataPointer();

            static inline ilen::ecs::ECSCoordinator* p_coordinator = nullptr;
            static inline ComponentArray<components::EntityMetadataComponent>* metadataPtr = nullptr;
    };
}