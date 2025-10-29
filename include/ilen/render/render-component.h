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
        std::weak_ptr<MeshGPULink> staticBuffers;
    };

    struct VariableRenderingData {
        std::shared_ptr<render::IGPUBuffer> uniformBuffer;
        uint32_t drawIndex = 0; //Indices de dibujo para subirlo a la GPU.
    };

    struct StaticMaterialRenderingData {
        StaticMaterialRenderingData() = default;
        std::weak_ptr<render::IGPURenderPipeline> pso;
    };

    struct VisibilityComponent {
        bool isVisible = true;
    };
} 