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
#include <ilen/render/renderer.h>
//A futuro assimp solo funcionará en el Editor para leer el archivo, prepararlo, y luego volcarlo como un binario.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
    using namespace ilen::core;    

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

                for (auto const& entity : entities) {
                    // 1. Si la entidad tiene un RenderComponent pero no un MeshGPULink, solicitamos la carga de la malla.
                    if (auto render = gCoordinator->getComponent<RenderComponent>(entity); render && !gCoordinator->hasComponent<MeshGPULink>(entity)) {
                        RequestMeshLoad request{};
                        request.meshPath = ilen::core::AssetsManager::getInstance().getAssetElement<ilen::core::ModelAsset>(render->meshID).assetPath;
                        gCoordinator->addComponent(entity, std::move(request));
                    }
    
                    // 2. Si la entidad tiene una solicitud de carga de malla, la procesamos.
                    if (auto meshRequest = gCoordinator->getComponent<RequestMeshLoad>(entity)) {
                        std::future<MeshData> future = jobSystemInstance->SubmitJob([meshPath = meshRequest->meshPath]() -> MeshData {
                            std::filesystem::path meshFilePath = core::FileManager::fileRoute(meshPath, ILEN_FILE_TYPE_NONE);
                            Assimp::Importer importer;
                            
                            const aiScene* scene = importer.ReadFile(meshFilePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
                            if(!scene || !scene->HasMeshes()){
                                Debug::Warning("El objeto en la ruta " + meshFilePath.string() + " no contenía ningún dato del tipo Mesh o ocurrió un error. Favor de revisar la información del archivo. Error: " + importer.GetErrorString(), "MESH_LOADING_IN_RENDER_SYSTEM");
                                return MeshData{};
                            }

                            //De momento soporte a un mesh por archivo. A futuro se debe de cambiar.
                            aiMesh* mesh = scene->mMeshes[0];

                            std::vector<render::buffers::VertexBufferData> vertexData;
                            std::vector<uint32_t> indexData;
                            
                            //Obteniendo los vertices
                            for(uint32_t i = 0; i < mesh->mNumVertices; i++){
                                render::buffers::VertexBufferData vertex{};
                                vertex.vertexPosition.x = mesh->mVertices[i].x;
                                vertex.vertexPosition.y = mesh->mVertices[i].y;
                                vertex.vertexPosition.z = mesh->mVertices[i].z;
                                vertex.vertexPosition.w = 1.0;

                                if(mesh->HasTextureCoords(0)){
                                    vertex.UV.x = mesh->mTextureCoords[0][i].x;
                                    vertex.UV.y = mesh->mTextureCoords[0][i].y;
                                }else{
                                    vertex.UV.x = vertex.UV.y = 0.0; //UV texture por defecto
                                }

                                if(mesh->HasVertexColors(0)){
                                    vertex.vertexColor.r = mesh->mColors[i]->r;
                                    vertex.vertexColor.g = mesh->mColors[i]->g;
                                    vertex.vertexColor.b = mesh->mColors[i]->b;
                                    vertex.vertexColor.a = mesh->mColors[i]->a;
                                }else{
                                    vertex.vertexColor = ilen::graphics::color::White;
                                }

                                vertexData.push_back(vertex);
                            }

                            //Obteniendo los indices para dibujar los datos.
                            for(uint32_t i = 0; i < mesh->mNumFaces; i++){
                                aiFace face = mesh->mFaces[i];
                                for(uint32_t j = 0; j < face.mNumIndices; j++){
                                    indexData.push_back(face.mIndices[j]);
                                }
                            }

                            MeshData meshData{};
                            meshData.indexBufferData = indexData;
                            meshData.vertexBufferData = vertexData;

                            return meshData;
                        });
                        
                        // Añadimos el componente que indica que estamos esperando la carga.
                        gCoordinator->addComponent(entity, WaitingForMeshLoad{entity, std::move(future)});        
                        // Eliminamos el componente de solicitud, ya no es necesario.
                        gCoordinator->removeComponent<RequestMeshLoad>(entity); // Se procesa la solicitud, se elimina.
                    }
                }
            }

            void Update(){
                // Este método se debe llamar en cada fotograma.
                // Revisa las mallas que están en proceso de carga.
                for(auto const& entity : entities){
                    //Comprobar si se cargaron los datos de una mesh
                    if(auto waiting = gCoordinator->getComponent<WaitingForMeshLoad>(entity); waiting != nullptr){
                        // std::future::wait_for() nos permite revisar el estado sin bloquear el hilo.
                        if(waiting->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
                            // La carga ha terminado. Obtenemos los datos.
                            MeshData meshData = waiting->future.get();
                            gCoordinator->addComponent(entity, std::move(meshData));
                            gCoordinator->removeComponent<WaitingForMeshLoad>(entity);
                        }
                    }

                    //Subir meshes a la GPU si aún no esta creado el link, y existen datos de mesh.
                    if(auto needToUpdloadToGPU = gCoordinator->getComponent<MeshData>(entity); needToUpdloadToGPU != nullptr && !gCoordinator->hasComponent<MeshGPULink>(entity)){
                        MeshGPULink link{};
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

                        gCoordinator->addComponent(entity, std::move(link));
                        gCoordinator->removeComponent<MeshData>(entity);
                    }   
                }
            }

        private:
            JobSystem* jobSystemInstance;            
    };
}