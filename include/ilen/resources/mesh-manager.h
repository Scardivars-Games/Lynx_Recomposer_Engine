//Cerebro principal de todas las operaciones relacionadas a las mallas. Administra que mallas están cargadas y los buffers asignados a cada malla. 
#pragma once

#include <ilen/job-system/job-system.h>
#include <ilen/render/renderer.h>
#include <ilen/core/lifetime.h>
#include <ilen/graphics/meshes.h>
#include <unordered_map>
#include <future>

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
    //using namespace ilen::core;
    using namespace ilen::graphics;

    using MeshID = uint32_t; //Solamente para legebilidad. Sigue siendo un uint32_t como el resto de IDs que usa el motor.
    using RuntimeMeshID = uint16_t;

    class MeshManager {
        public:
            explicit MeshManager(EngineContext* engineContext);

            //Porque weak_ptr? Porque el manejer puede eliminar por alguna razon el buffer de la GPU. En ese caso el render system, al hacer .lock() 
            //al weak_ptr para obtner un shared_ptr valido y trabajar con él, en caso de estar destruido obtendrá un nullptr, y así puede solicitar un recacheo del puntero.
            std::weak_ptr<MeshGPULink> getMeshHandlers(MeshID id);
            void UpdateProcess();
            uint16_t getMeshRuntimeID(MeshID id) const;
        private:
            std::unordered_map<MeshID, std::shared_ptr<MeshGPULink>> m_meshBufferData;
            std::unordered_map<MeshID, std::future<MeshGPULink>> m_meshLoadingData;
            std::unordered_map<MeshID, RuntimeMeshID> m_meshRuntime;

            JobSystem* jobSystem;
            AssetsManager* assetsManager;
            IRenderer* renderer;
            mutable std::mutex m_mutex;

            uint16_t m_nextRuntimeID = 1;

            const char* DEBUG_PREFIX = "MESH_MANAGER";
    };
}