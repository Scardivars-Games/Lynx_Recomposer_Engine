#pragma once

#include <cstdint>
#include <ilen/headers/ecs.h>
#include <filesystem>
#include <vector>
#include <ilen/render/vertex-buffer.h>

namespace ilen::ecs::components {
    
    struct RenderComponent {
        uint32_t meshID;
        uint32_t mainTexID;
        uint32_t materialID;   
    };

    //Si una entidad recibe esta estructura, significa que debe cargar la mesh en la ruta indicada. Operación destinada a un JobSystem. Debe eliminarse una vez inyectado un componente con los datos.
    struct RequestMeshLoad {
        std::filesystem::path meshPath;
    };

    //Indica que una entidad precisa de cargar sus datos a la VRAM. Debe eliminarse una vez se ha hecho la carga.
    struct MeshData {
        std::vector<render::buffers::VertexBuffer> vertexBufferData; //Array de los vertices del objeto a ser utilizados.
        std::vector<uint32_t> indexBufferData;
    };

    struct MeshGPULink {

    };
}