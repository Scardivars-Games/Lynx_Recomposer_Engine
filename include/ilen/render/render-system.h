#pragma once

#include <ilen/core/assets-manager.h>
#include <ilen/ecs/types.h>
#include <ilen/headers/enumerations.h>
#include <cstddef>
#include <ilen/render/render-component.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <vector>
#include <ilen/render/renderer.h>
#include <ilen/resources/mesh-manager.h>

namespace ilen::core {
    using namespace ilen::enumerations;

    template <ILEN_ASSET_TYPE T>
    struct AssetElement;

    using ModelAsset = AssetElement<ILEN_ASSET_TYPE_MODEL>;
}

namespace ilen::ecs::systems {

    using namespace ilen::resources::mesh;
    using namespace ilen::ecs::components; 
    using namespace ilen::job_system;    
    using namespace ilen::core;    

    class RenderSystem : public ilen::ecs::System {
        public:
            void Start(EngineContext* engineContext) {
                m_jobSystemInstance = engineContext->p_jobSystem.get();
                m_meshManager = engineContext->p_meshManager.get();
                m_Coordinator = engineContext->p_coordinator.get();
            }

            void Update(){
                // Este método se debe llamar en cada fotograma.
                // Revisa las mallas que están en proceso de carga.
                for(auto const& entity : entities){
                    //Comprobar si se cargaron los datos de una mesh
                    if(auto waiting = m_Coordinator->getComponent<WaitingForMeshLoad>(entity); waiting != nullptr){
                        // std::future::wait_for() nos permite revisar el estado sin bloquear el hilo.
                        if(waiting->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
                            // La carga ha terminado. Obtenemos los datos.
                            MeshData meshData = waiting->future.get();
                            m_Coordinator->addComponent(entity, std::move(meshData));
                            m_Coordinator->removeComponent<WaitingForMeshLoad>(entity);
                        }
                    }

                    //Subir meshes a la GPU si aún no esta creado el link, y existen datos de mesh.
                    if(auto needToUpdloadToGPU = m_Coordinator->getComponent<MeshData>(entity); needToUpdloadToGPU != nullptr && !m_Coordinator->hasComponent<MeshGPULink>(entity)){
                       /* MeshGPULink link{};
                        auto& renderer = ilen::AppRuntime::getInstance().getGameRenderer();
                        
                        ilen::render::IlenBufferDescriptor vertexDescriptor{};
                        vertexDescriptor.bufferType = ILEN_RENDER_GPU_BUFFER_VERTEX;
                        vertexDescriptor.bufferSize = needToUpdloadToGPU->vertexBufferData.size() * sizeof(ilen::render::buffers::VertexBufferData);
                        vertexDescriptor.cpuVisible = false;
                        vertexDescriptor.count = needToUpdloadToGPU->vertexBufferData.size();

                        link.vertexBuffer = renderer.createBuffer(vertexDescriptor, needToUpdloadToGPU->vertexBufferData.data());

                        ilen::render::IlenBufferDescriptor indexDescriptor{};
                        indexDescriptor.bufferType = ILEN_RENDER_GPU_BUFFER_INDEX;
                        indexDescriptor.bufferSize = needToUpdloadToGPU->indexBufferData.size() * sizeof(uint32_t);
                        indexDescriptor.cpuVisible = false;
                        indexDescriptor.count = needToUpdloadToGPU->indexBufferData.size();

                        link.indexBuffer = renderer.createBuffer(indexDescriptor, needToUpdloadToGPU->indexBufferData.data());

                        m_Coordinator->addComponent(entity, std::move(link));
                        m_Coordinator->removeComponent<MeshData>(entity);*/
                    }   
                }
            }

        private:
            ilen::ecs::ECSCoordinator* m_Coordinator;
            JobSystem* m_jobSystemInstance;   
            MeshManager* m_meshManager;   
            const char* DEBUG_PREFIX = "RENDER_SYSTEM";
    };
}