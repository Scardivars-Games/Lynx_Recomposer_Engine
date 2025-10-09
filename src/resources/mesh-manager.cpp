#include <ilen/resources/mesh-manager.h>

namespace ilen::resources::mesh {
    MeshManager::MeshManager(EngineContext* engineContext){
        jobSystem = engineContext->p_jobSystem.get();
        renderer = engineContext->p_gameRenderer.get();
        assetsManager = engineContext->p_assetsManager.get();
    }


    std::weak_ptr<MeshGPULink> MeshManager::getMeshHandlers(MeshID id) {
        //Necesito realizar lo siguiente:
        //1- Si no existe la mesh ya en la GPU, revisar si no se esta cargando.
        //2 - Si no se esta cargando, entonces crear un proceso de carga usando JobSystem

        auto it = m_meshBufferData.find(id);
        if(it != m_meshBufferData.end()){
            return it->second; //Regreso un weak_ptr a partir del shared_ptr existent :)
        }

        auto isLoading = m_meshLoadingData.find(id);
        if(isLoading != m_meshLoadingData.end()){
            return {}; //Se esta cargando, regresamos una estructura sin datos para que falle al lockearla desde el cliente
        }

        std::filesystem::path meshPath = assetsManager->getAssetElement<ModelAsset>(id).assetPath;

        //Cargando los datos a la GPU.
        std::future<MeshGPULink> future = jobSystem->SubmitJob([this, meshPath]() -> MeshGPULink {
                //TODO: Load binary data of mesh
                auto loadPath = ilen::core::FileManager::fileRoute(meshPath, ILEN_FILE_TYPE_MODEL);
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

                MeshGPULink link{};
                
                ilen::render::IlenBufferDescriptor vertexDescriptor{};
                vertexDescriptor.bufferType = ILEN_RENDER_GPU_BUFFER_VERTEX;
                vertexDescriptor.bufferSize = mesh.vertexBufferData.size() * sizeof(ilen::render::buffers::VertexBufferData);
                vertexDescriptor.cpuVisible = false;
                vertexDescriptor.count = mesh.vertexBufferData.size();

                link.vertexBuffer = renderer->createBuffer(vertexDescriptor, mesh.vertexBufferData.data());

                ilen::render::IlenBufferDescriptor indexDescriptor{};
                indexDescriptor.bufferType = ILEN_RENDER_GPU_BUFFER_INDEX;
                indexDescriptor.bufferSize = mesh.indexBufferData.size() * sizeof(uint32_t);
                indexDescriptor.cpuVisible = false;
                indexDescriptor.count = mesh.indexBufferData.size();

                link.indexBuffer = renderer->createBuffer(indexDescriptor, mesh.indexBufferData.data());
                return link;
            });

        m_meshLoadingData.emplace(id, std::move(future));
        return {};
    }

    void MeshManager::UpdateProcess() {
        for(auto it = m_meshLoadingData.begin(); it != m_meshLoadingData.end();){
            auto& [id, future] = *it;

            if(future.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
                //Obtenemos la promesa futura y almacenamos en la estructura correspondiente.
                MeshGPULink link = future.get();
                m_meshBufferData[id] = std::make_shared<MeshGPULink>(std::move(link));

                it = m_meshLoadingData.erase(it); //Elimino el iterador actual, y registro el próximo disponible.
            }else{
                ++it;
            }
        }
    }
}