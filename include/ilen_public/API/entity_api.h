#pragma once

#include <ilen/headers/ecs.h>
#include <ilen/game_logic/script_system.h>
#include <ilen/core/engine_context.h>
#include "ilen/ecs/types.h"

using namespace ilen::ecs;

namespace ilen::engine::api {
    //API diseñada para interactuar con las entidades del ECS. Esto lo conseguira mediante el MetadataComponent, por lo que lo modificara, lo analizara, y retornara entidades.
    class EntityAPI {
        friend class systems::ScriptingSystem;

        public:
            //Propiedades en los metadatos de la entidad
            static void SetActive(Entity entity, bool newState);
            static bool isActive(Entity entity);
            static Entity GetEntityByName(std::string entityName);
            static bool entityHasTag(Entity entity, TagSystem tag);
            static std::string GetEntityName(Entity entity);

            //Prefabs
            static void LoadPrefab(std::string prefabName);
            static void LoadPrefab(std::string prefabName, vec3 prefabPosition);
        private:
            static void Init(const ilen::EngineContext* context);
            static void ResetMetadataPointer(); //Resetea el puntero de los metadatos, para obtener uno válido el próximo frame.
            static inline void GetMetadataPointer();

            static inline ilen::resources::scene::SceneManager* p_sceneManager = nullptr;
            static inline ilen::ecs::ECSCoordinator* p_coordinator = nullptr;
            static inline ComponentArray<components::EntityMetadataComponent>* metadataPtr = nullptr;
    };
}