//Cerebro principal de todas las operaciones relacionadas a las mallas. Administra que mallas están cargadas y los buffers asignados a cada malla. 
#pragma once

#include <lynx/job_system/job_system.h>
#include <lynx/render/renderer.h>
#include <lynx/core/lifetime.h>
#include <lynx/graphics/meshes.h>
#include <unordered_map>
#include <future>

namespace lynx::resources {
    class AssetsManager;
}

namespace lynx {
    struct EngineContext;
}

namespace lynx::render
{
    class IRenderer;
}

namespace lynx::resources::mesh {
    //Almacena los datos necesarios para las llamadas a la GPU.
    using namespace lynx::job_system;
    using namespace lynx::render;
    //using namespace lynx::core;
    using namespace lynx::graphics;

    using MeshID = uint32_t; //Solamente para legebilidad. Sigue siendo un uint32_t como el resto de IDs que usa el motor.
    using RuntimeMeshID = uint16_t;

    class MeshManager {
        public:
            explicit MeshManager(EngineContext* engineContext);

            //Porque weak_ptr? Porque el manejer puede eliminar por alguna razon el buffer de la GPU. En ese caso el render system, al hacer .lock() 
            //al weak_ptr para obtner un shared_ptr valido y trabajar con él, en caso de estar destruido obtendrá un nullptr, y así puede solicitar un recacheo del puntero.
            std::weak_ptr<MeshGPULink> getMeshHandlers(MeshID id);
            void UpdateProcess();
            float LoadingStatus();
            uint16_t getMeshRuntimeID(MeshID id) const;
        private:
            std::unordered_map<MeshID, std::shared_ptr<MeshGPULink>> m_meshBufferData;
            std::unordered_map<MeshID, std::future<MeshGPULink>> m_meshLoadingData;
            std::unordered_map<MeshID, RuntimeMeshID> m_meshRuntime;

            uint32_t lastTimeAmountOfElementLoading = 0;

            JobSystem* jobSystem;
            AssetsManager* assetsManager;
            IRenderer* renderer;
            mutable std::mutex m_mutex;

            uint16_t m_nextRuntimeID = 1;

            const char* DEBUG_PREFIX = "MESH_MANAGER";
    };
}