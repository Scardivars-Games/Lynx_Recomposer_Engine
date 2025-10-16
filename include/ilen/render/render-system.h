#pragma once

#include <ilen/resources/assets-manager.h>
#include <ilen/ecs/types.h>
#include <ilen/headers/enumerations.h>
#include <cstddef>
#include <ilen/render/render-component.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <vector>
#include <ilen/render/renderer.h>
#include <ilen/resources/mesh-manager.h>

namespace ilen::resources {
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
                m_meshManager->UpdateProcess();
                // Este método se debe llamar en cada fotograma.
                for(auto const& entity : entities){ 
                    if(auto render = m_Coordinator->getComponent<RenderComponent>(entity); render != nullptr){
                        //En este loops estamos trabajando con una entidad con rendering. Toda entidad que renderice tiene que tener sus datos en VRAM, sino, hay que asignarlos
                        auto staticMeshData = m_Coordinator->getComponent<StaticRenderingData>(entity);
                        
                        if(staticMeshData == nullptr){
                           //Solicitar la carga de estos datos
                            auto solitudeResult = m_meshManager->getMeshHandlers(render->meshID);

                            if(solitudeResult.lock() == nullptr){
                                continue; //Salta a la siguiente entidad, esta no puede hacer nada.
                            }else{  
                                //Guardamos el puntero
                                StaticRenderingData data{};
                                data.staticBuffers = solitudeResult;
                                m_Coordinator->addComponent(entity, std::move(data));
                            }
                        }else{
                            auto pointer = staticMeshData->staticBuffers.lock();
                            if(pointer == nullptr){
                                //Ya no existe, borramos el componente
                                m_Coordinator->removeComponent<StaticRenderingData>(entity);
                                continue;
                            }
                            //Puntero es válido a datos de la GPU.
                        }
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