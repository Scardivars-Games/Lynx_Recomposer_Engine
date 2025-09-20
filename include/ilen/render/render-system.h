#pragma once

#include <ilen/core/assets-manager.h>
#include <ilen/ecs/types.h>
#include <ilen/headers/enumerations.h>
#include <cstddef>
#include <ilen/render/render-component.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <vector>
#include <ilen/job-system/job-system.h>

extern std::unique_ptr<ilen::ecs::ECSCoordinator> gCoordinator;

namespace ilen::core {
    using namespace ilen::enumerations;

    template <ILEN_ASSET_TYPE T>
    struct AssetElement;

    using ModelAsset = AssetElement<ILEN_ASSET_TYPE_MODEL>;
}

namespace ilen::ecs::systems {

    using namespace ilen::ecs::components; 
    using namespace ilen::job_system;        

    class RenderSystem : public ilen::ecs::System {
        public:
            void Start(){
                jobSystemInstance = &JobSystem::getInstance();
                //Inicializar el sistema de renderizado.
                //Pasos:
                //1 - Buscar objetos que tengos un RenderComponent.
                //2 - Si esos objetos no tienen MeshGPULink, significa que sus datos no estan en la GPU. Asignarles un RequestMeshLoad.
                //3 - Tras eso, usamos el job-system para cargar la mesh en su estructura de datos.
                //4 - Enviamos todos estos datos a la GPU según la API gráfica.

                for(auto const& entity : entities){
                    auto render = gCoordinator->getComponent<RenderComponent>(entity);
                    if(render != nullptr){
                        if(gCoordinator->getComponent<MeshGPULink>(entity) == nullptr){
                            //No existe el vínculo entre entidad y GPU, asignando que se lean sus datos del mesh.
                            RequestMeshLoad request{};
                            request.meshPath = ilen::core::AssetsManager::getInstance().getAssetElement<ilen::core::ModelAsset>(render->meshID).assetPath;
                            gCoordinator->addComponent(entity, std::move(request));
                        }
                    }
                }

                //Leyendo datos del mesh
                for(auto const& entity: entities){
                    auto meshRequest = gCoordinator->getComponent<RequestMeshLoad>(entity);
                    if(meshRequest != nullptr){
                        std::future<MeshData> future = jobSystemInstance->SubmitJob([meshPath = meshRequest->meshPath]() -> MeshData {
                            // TODO: Implement mesh loading logic from meshPath
                            // For now, returning an empty MeshData object.
                            return MeshData{};
                        });
                        
                        // Añadimos el componente que indica que estamos esperando la carga.
                        gCoordinator->addComponent(entity, WaitingForMeshLoad{entity, std::move(future)});        
                        // Eliminamos el componente de solicitud, ya no es necesario.
                        gCoordinator->removeComponent<RequestMeshLoad>(entity);
                    }
                }
            }

            void Update(){
                // Este método se debe llamar en cada fotograma.
                // Revisa las mallas que están en proceso de carga.
                for(auto const& entity : entities){
                    auto waiting = gCoordinator->getComponent<WaitingForMeshLoad>(entity);
                    if(waiting != nullptr){
                        // std::future::wait_for() nos permite revisar el estado sin bloquear el hilo.
                        if(waiting->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
                            // La carga ha terminado. Obtenemos los datos.
                            MeshData meshData = waiting->future.get();
                            gCoordinator->addComponent(entity, std::move(meshData));
                            gCoordinator->removeComponent<WaitingForMeshLoad>(entity);
                        }
                    }
                }
            }

        private:
            JobSystem* jobSystemInstance;            
    };
}