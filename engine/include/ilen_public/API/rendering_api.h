#pragma once

//API publica para todo lo relacionado con rendering
//Esto incluye, modificar propiedades de los componentes de rendering (Mesh, Texture, Material, etc)
//O enviar datos a los shaders de la GPU, modificando los buffers.

#include <ilen/headers/ecs.h>
#include <ilen/render/render_system.h>
#include <ilen/core/engine_context.h>
#include <ilen/render/render_component.h>

using namespace ilen::ecs;

namespace ilen::engine::api {
    //API diseñada para interactuar con las entidades del ECS. Esto lo conseguira mediante el MetadataComponent, por lo que lo modificara, lo analizara, y retornara entidades.
    struct RenderingComponents{
        ComponentArray<components::RenderComponent>* renderComponentArray = nullptr;
        ComponentArray<components::StaticMaterialRenderingData>* staticMaterialArray = nullptr;
        ComponentArray<components::StaticRenderingData>* staticRenderingArray = nullptr;
        ComponentArray<components::StaticTextureRenderingData>* staticTextureArray = nullptr;
    };

    class RenderingAPI {
        friend class systems::ScriptingSystem;

        public:
            static void FlipSpriteInX(Entity entity, bool value);
            static void FlipSpriteInY(Entity entity, bool value);
            static void ChangeSpriteIndex(Entity entity, int newIndex);
        private:
            static void Init(const ilen::EngineContext* context);
            static void ResetComponentsPointers(); //Resetea el puntero de los metadatos, para obtener uno válido el próximo frame.
            static inline void GetComponentPointers();

            static inline ilen::ecs::ECSCoordinator* p_coordinator = nullptr;
            static inline RenderingComponents components;
            const static inline char* DEBUG_PREFIX = "RENDERING_API";
    };
}