#pragma once

#include <cstdint>
#include <ilen/headers/ecs.h>
#include <filesystem>
#include <vector>
#include <future>
#include <ilen/render/renderer.h>
#include <ilen/resources/mesh-manager.h>
#include <ilen/graphics/meshes.h>
/*
namespace ilen::graphics {
    struct MeshGPULink;
}*/

namespace ilen::ecs::components {
    
    using namespace ilen::graphics;

    struct RenderComponent {
        uint32_t meshID;
        uint32_t mainTexID;
        uint32_t materialID;   
    };

    //Puntero debil a los buffers fijos en la GPU. Deben solicitarse al MeshManager el puntero y antes de operar hacer un .lock() para obtener un shared_ptr válido para ese frame. En que de que no se obtenga algo válido, volver a solicitar los meshes.
    struct StaticRenderingData {
        StaticRenderingData() = default;
        StaticRenderingData(std::weak_ptr<MeshGPULink> buffer, uint32_t runtimeID) : staticBuffers(buffer), runtimeMeshID(runtimeID) {}
        std::weak_ptr<MeshGPULink> staticBuffers = {};
        uint32_t runtimeMeshID = 0;
    };

    struct VariableRenderingData {
        std::shared_ptr<render::IGPUBuffer> uniformBuffer;
        uint32_t drawIndex = 0; //Indices de dibujo para subirlo a la GPU.
    };

    struct StaticMaterialRenderingData {
        StaticMaterialRenderingData() = default;
        StaticMaterialRenderingData(std::weak_ptr<render::IGPURenderPipeline> pso, uint32_t runtimeID) : pso(pso), runtimeMaterialID(runtimeID) {};
        std::weak_ptr<render::IGPURenderPipeline> pso;
        uint32_t runtimeMaterialID = 0;
    };

    struct VisibilityComponent {
        bool isVisible = true;
    };

    //No es un componente como tal del ECS, pero si una estructura necesaria para poder renderizar
    struct RenderItem {
        Entity entity;
        uint64_t sortKey;

        bool operator<(const RenderItem& other) const {
            return sortKey < other.sortKey;
        }
    };
} 