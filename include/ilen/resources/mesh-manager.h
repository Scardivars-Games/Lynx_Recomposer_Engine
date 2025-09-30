//Cerebro principal de todas las operaciones relacionadas a las mallas. Administra que mallas están cargadas y los buffers asignados a cada malla. 
#pragma once

#include <ilen/job-system/job-system.h>
#include <ilen/render/renderer.h>
#include <unordered_map>
#include <future>


namespace ilen::resources::mesh {
    //Almacena los datos necesarios para las llamadas a la GPU.
    using namespace ilen::job_system;
    using namespace ilen::render;
    using MeshID = uint32_t; //Solamente para legebilidad. Sigue siendo un uint32_t como el resto de IDs que usa el motor.

    struct MeshGPULink {
        std::shared_ptr<render::IGPUBuffer> vertexBuffer;
        std::shared_ptr<render::IGPUBuffer> indexBuffer;
    };

    class MeshManager {
        public:

        private:
            std::unordered_map<MeshID, MeshGPULink> m_meshBufferData;
            std::unordered_map<MeshID, std::future<MeshGPULink>> m_meshLoadingData;

            JobSystem* jobSystem;
            Renderer* renderer;
    };
}

/*
std::future<MeshData> future = jobSystemInstance->SubmitJob([this, meshPath = meshRequest->meshPath]() -> MeshData {
    //TODO: Load binary data of mesh
    auto loadPath = FileManager::fileRoute(meshPath, ILEN_FILE_TYPE_MODEL);
    //ILEN_LOG_P(DEBUG_PREFIX, "Mesh localizado en %s.", loadPath.c_str());
    
    std::ifstream readMesh(loadPath, std::ios::binary);
    if(!readMesh.is_open())
        ILEN_ERROR_P(DEBUG_PREFIX, "No se pudo abrir el mesh ubicado en %s.", loadPath.c_str());                            

    ilen::binary::headers::IlenMeshHeader meshHeader{};
    readMesh.read(reinterpret_cast<char*>(&meshHeader), sizeof(meshHeader));

    if(std::string(meshHeader.magicNumber) != "HSEM_NELI")
        ILEN_ERROR_P(DEBUG_PREFIX, "La cabezera del mesh ubicado en %s no es compatible con este programa", loadPath.c_str());

    MeshData mesh{};
    mesh.vertexBufferData.resize(meshHeader.vertexCount);
    mesh.indexBufferData.resize(meshHeader.indexCount);

    readMesh.read(reinterpret_cast<char*>(mesh.vertexBufferData.data()), sizeof(ilen::render::buffers::VertexBufferData) * meshHeader.vertexCount);
    readMesh.read(reinterpret_cast<char*>(mesh.indexBufferData.data()), sizeof(uint32_t) * meshHeader.indexCount);

    readMesh.close();
    return mesh;
});
*/