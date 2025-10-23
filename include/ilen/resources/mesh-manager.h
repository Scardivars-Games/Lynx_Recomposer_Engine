//Cerebro principal de todas las operaciones relacionadas a las mallas. Administra que mallas están cargadas y los buffers asignados a cada malla. 
#pragma once

#include <ilen/job-system/job-system.h>
#include <ilen/render/gpu-interfaces/buffers.h>
#include <ilen/render/renderer.h>
#include <ilen/resources/assets-manager.h>
#include <ilen/core/filemanager.h>
#include <ilen/core/lifetime.h>
#include <ilen/headers/ilen-logs.h>
#include <unordered_map>
#include <future>
#include <chrono>

namespace ilen::resources {
    class AssetsManager;
}

namespace ilen {
    struct EngineContext;
}

namespace ilen::render
{
    class IRenderer;
}

namespace ilen::resources::mesh {
    //Almacena los datos necesarios para las llamadas a la GPU.
    using namespace ilen::job_system;
    using namespace ilen::render;
    using namespace ilen::core;

    using MeshID = uint32_t; //Solamente para legebilidad. Sigue siendo un uint32_t como el resto de IDs que usa el motor.

    //Indica que una entidad precisa de cargar sus datos a la VRAM. Debe eliminarse una vez se ha hecho la carga.
    struct MeshData {
        std::vector<ilen::render::buffers::VertexBufferData> vertexBufferData; //Array de los vertices del objeto a ser utilizados.
        std::vector<uint32_t> indexBufferData;
    };

    struct MeshGPULink {
        std::shared_ptr<render::IGPUBuffer> vertexBuffer;
        std::shared_ptr<render::IGPUBuffer> indexBuffer;
    };

    class MeshManager {
        public:
            MeshManager(EngineContext* engineContext);

            //Porque weak_ptr? Porque el manejer puede eliminar por alguna razon el buffer de la GPU. En ese caso el render system, al hacer .lock() 
            //al weak_ptr para obtner un shared_ptr valido y trabajar con él, en caso de estar destruido obtendrá un nullptr, y así puede solicitar un recacheo del puntero.
            std::weak_ptr<MeshGPULink> getMeshHandlers(MeshID id);
            void UpdateProcess();
        private:
            std::unordered_map<MeshID, std::shared_ptr<MeshGPULink>> m_meshBufferData;
            std::unordered_map<MeshID, std::future<MeshGPULink>> m_meshLoadingData;

            JobSystem* jobSystem;
            AssetsManager* assetsManager;
            IRenderer* renderer;

            const char* DEBUG_PREFIX = "MESH_MANAGER";
    };
}