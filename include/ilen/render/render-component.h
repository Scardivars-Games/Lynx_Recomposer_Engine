#pragma once

#include <cstdint>
#include <ilen/headers/ecs.h>
#include <filesystem>
#include <vector>
#include <future>
#include <ilen/render/renderer.h>
#include <ilen/resources/mesh-manager.h>
#include <ilen/graphics/meshes.h>
#include <ilen/graphics/texture.h>
#include <ilen/core/runtime_options.h>

/*
namespace ilen::graphics {
    struct MeshGPULink;
}*/

namespace ilen::ecs::components {
    
    using namespace ilen::graphics;
    using namespace ilen::core::options;

    ILEN_COMPONENT
    struct RenderComponent {
        ILEN_PROPERTY uint32_t meshID = PredefinedAssetsID::QuadMeshID;
        ILEN_PROPERTY uint32_t mainTexID = PredefinedAssetsID::DefaultTextureID;
        ILEN_PROPERTY uint32_t materialID = PredefinedAssetsID::DefaultUnlitOpaqueMatID;
        ILEN_PROPERTY uint32_t defaultSpriteInTextureID = 0;
    };

    //Se genera en tiempo de ejecución, NO ES UN COMPONENTE A SERIALIZAR
    //Puntero debil a los buffers fijos en la GPU. Deben solicitarse al MeshManager el puntero y antes de operar hacer un .lock() para obtener un shared_ptr válido para ese frame. En que de que no se obtenga algo válido, volver a solicitar los meshes.
    struct StaticRenderingData {
        StaticRenderingData() = default;
        StaticRenderingData(std::weak_ptr<MeshGPULink> buffer, uint32_t runtimeID) : staticBuffers(buffer), runtimeMeshID(runtimeID) {}
        std::weak_ptr<MeshGPULink> staticBuffers = {};
        uint32_t runtimeMeshID = 0;
    };

    //Se genera en tiempo de ejecución, NO ES UN COMPONENTE A SERIALIZAR
    struct VariableRenderingData {
        std::shared_ptr<render::IGPUBuffer> uniformBuffer;
        uint32_t drawIndex = 0; //Indices de dibujo para subirlo a la GPU.
    };

    //Se genera en tiempo de ejecución, NO ES UN COMPONENTE A SERIALIZAR
    struct StaticMaterialRenderingData {
        StaticMaterialRenderingData() = default;
        StaticMaterialRenderingData(std::weak_ptr<render::IGPURenderPipeline> pso, uint32_t runtimeID) : pso(pso), runtimeMaterialID(runtimeID) {};
        std::weak_ptr<render::IGPURenderPipeline> pso;
        uint32_t runtimeMaterialID = 0;
    };

    //Se genera en tiempo de ejecución, NO ES UN COMPONENTE A SERIALIZAR
    struct StaticTextureRenderingData {
        //Porque tener esto separado??? Para que los elementos claves usandos al renderizar sean combinados de la forma más sana posible durante el sorting al renderizar.
        StaticTextureRenderingData() = default;
        StaticTextureRenderingData(std::weak_ptr<TextureGPULink> tex, uint32_t runtimeID, uint32_t startSpriteID)
            : staticTexture(tex), runtimeTexID(runtimeID), currentSpriteID(startSpriteID) {}
        
        std::weak_ptr<TextureGPULink> staticTexture = {};
        uint32_t runtimeTexID = 0;
        uint32_t currentSpriteID = 0; //Es el ID del sprite en uso de la textura. Esto debe ser modificado por algunos sistemas como el de animcacion usando alguna API.

        //Flip en X o Y
        bool flipX = false;
        bool flipY = false;
    };

    //Se genera en tiempo de ejecución, NO ES UN COMPONENTE A SERIALIZAR
    struct VisibilityComponent {
        bool isVisible = true;
    };

    //Se genera en tiempo de ejecución, NO ES UN COMPONENTE A SERIALIZAR
    //No es un componente como tal del ECS, pero si una estructura necesaria para poder renderizar
    struct RenderItem {
        Entity entity;
        uint64_t sortKey;

        bool operator<(const RenderItem& other) const {
            return sortKey < other.sortKey;
        }
    };
} 