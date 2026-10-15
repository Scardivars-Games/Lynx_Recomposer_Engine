#pragma once

//API publica para todo lo relacionado con rendering
//Esto incluye, modificar propiedades de los componentes de rendering (Mesh, Texture, Material, etc)
//O enviar datos a los shaders de la GPU, modificando los buffers.

#include <lynx/headers/ecs.h>
#include <lynx/render/render_system.h>
#include <lynx/core/engine_context.h>
#include <lynx/render/render_component.h>

using namespace lynx::ecs;

namespace lynx::engine::api {
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
            static void Init(const lynx::EngineContext* context);
            static void ResetComponentsPointers(); //Resetea el puntero de los metadatos, para obtener uno válido el próximo frame.
            static inline void GetComponentPointers();

            static inline lynx::ecs::ECSCoordinator* p_coordinator = nullptr;
            static inline RenderingComponents components;
            const static inline char* DEBUG_PREFIX = "RENDERING_API";
    };
}