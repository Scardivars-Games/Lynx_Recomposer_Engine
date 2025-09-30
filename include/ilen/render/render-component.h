#pragma once

#include <cstdint>
#include <ilen/headers/ecs.h>
#include <filesystem>
#include <vector>
#include <ilen/render/vertex-buffer.h>
#include <future> 
#include <ilen/render/renderer.h>

namespace ilen::ecs::components {
    
    struct RenderComponent {
        uint32_t meshID;
        uint32_t mainTexID;
        uint32_t materialID;   
    };

    //Indica que una entidad precisa de cargar sus datos a la VRAM. Debe eliminarse una vez se ha hecho la carga.
    struct MeshData {
        std::vector<render::buffers::VertexBufferData> vertexBufferData; //Array de los vertices del objeto a ser utilizados.
        std::vector<uint32_t> indexBufferData;
    };

    //Si una entidad recibe esta estructura, significa que debe cargar la mesh en la ruta indicada. Operación destinada a un JobSystem. Debe eliminarse una vez inyectado un componente con los datos.
    struct RequestMeshLoad {
        std::filesystem::path meshPath;
    };

    //Se usa para indicar que se esta procesado la lectura y escritura de los datos.
    struct WaitingForMeshLoad {
        ilen::ecs::Entity entity;
        std::future<MeshData> future;

        // 1. Constructor por defecto explícito.
        WaitingForMeshLoad() = default;

        // 2. Constructor para inicializar con valores.
        WaitingForMeshLoad(ilen::ecs::Entity e, std::future<MeshData>&& f)
            : entity(e), future(std::move(f)) {}

        // 3. Constructores de movimiento y eliminación de copia explícitos.
        WaitingForMeshLoad(const WaitingForMeshLoad&) = delete;
        WaitingForMeshLoad& operator=(const WaitingForMeshLoad&) = delete;
        WaitingForMeshLoad(WaitingForMeshLoad&&) noexcept = default;
        WaitingForMeshLoad& operator=(WaitingForMeshLoad&&) noexcept = default;
    };

    struct VariableRenderingData {
        std::shared_ptr<render::IGPUBuffer> uniformBuffer;
        uint32_t drawIndex = 0; //Indices de dibujo para subirlo a la GPU.
    };
} 