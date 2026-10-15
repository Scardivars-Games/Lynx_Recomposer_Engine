#include <lynx/render/render_draw_system.h>
#include <lynx/render/render_constants.h>
//#include <lynx/core/runtime_options.h>
#include <lynx/transform/transform_component.h>
#include <lynx/core/engine_context.h>
#include <lynx/render/renderer.h>
#include <lynx/render/render_component.h>
#include <lynx/camera/camera_system.h>

namespace lynx::ecs::systems {

    using namespace lynx::maths::matrix;
    using namespace lynx::ecs::components;

    void RenderDrawSystem::Init(const EngineContext* engineContext) {
        p_coordinator = engineContext->p_coordinator.get();
        p_jobSystem = engineContext->p_jobSystem.get();
        p_Renderer = engineContext->p_gameRenderer.get();
        p_matManager = engineContext->p_materialManager.get();
        p_meshManager = engineContext->p_meshManager.get();
//      p_cameraSystem = engineContext->p_cameraSystem.get();
        p_cameraSystem = p_coordinator->getSystem<CameraSystem>().get();

    }

    void RenderDrawSystem::Update() {
        //Se saca del timer de opque porque el _drawable esta lockeado al VSyn en metal, y pueda adicionar tiempo adicional.
        
        std::unique_ptr<IGPUParallelCommandEncoder> encoder;
        {
            lynx::time::ScopedTimer timer("Time to get _drawable in Metal");
            //Obtener el encoder (paraleliza el trabajo)
            encoder = p_Renderer->getParallelCommandEncoder();
        }
        if (!encoder) {
            return; //No hay un encoder al que llamar para dibujar
        }
        auto encoderPtr = encoder.get();


        {
            lynx::time::ScopedTimer timer("Opaque screen rendering");
            auto* instanceBuffer = p_Renderer->getInstanceBuffer();
            if (instanceBuffer == nullptr) {
                LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "Renderer devolvio un Instance Buffer incorrecto. La aplicación no puede continuar su ejecución");
            }

            //Obtener camara activa
            auto activeCamera = p_cameraSystem->getMainCamera();
            if (!activeCamera) {
                LYNX_WARNING_P(DEBUG_PREFIX, "No se encontró una cámara activa, por lo que no se puede dibujar.");
                p_Renderer->returnParallelCommandEncoder(std::move(encoder));
                return; //Sin camara, no puedo dibujar.
            }

            //Obtener punteros a los array de los componentes que requiere el sistema para trabajar
            auto* transformComponentArray = p_coordinator->getComponentArray<TransformComponent>();
            auto* staticMeshArray = p_coordinator->getComponentArray<StaticRenderingData>();
            auto* staticMaterialArray = p_coordinator->getComponentArray<StaticMaterialRenderingData>();
            auto* visibleArray = p_coordinator->getComponentArray<VisibilityComponent>();
            auto* staticTextureArray = p_coordinator->getComponentArray<StaticTextureRenderingData>();

            if(!transformComponentArray || !staticMeshArray || !staticMaterialArray || !visibleArray || !staticTextureArray){
                p_Renderer->returnParallelCommandEncoder(std::move(encoder));
                return; //Es imposible ejecutar este sistema si esos componentes aún no existen.
            }

            lastRenderItems.clear();
            std::vector<RenderItem> renderItems;
            renderItems.reserve(entities.size());

            //ECS no manda entidades a este objeto. Problema de firmas, seguramente.
            //Corregido :)
            /*for (auto const& entity: entities) {
                auto* transform = p_coordinator->getComponent<TransformComponent>(entity);
                auto* staticMesh = p_coordinator->getComponent<StaticRenderingData>(entity);
                auto* staticMaterial = p_coordinator->getComponent<StaticMaterialRenderingData>(entity);
                auto* visible = p_coordinator->getComponent<VisibilityComponent>(entity);

                if (!transform || !staticMesh || !staticMaterial || !visible) {
                    continue; //No cumple los requisitos para poder dibujarse en pantalla
                }

                if (staticMesh->staticBuffers.lock() == nullptr || staticMaterial->pso.lock() == nullptr) {
                    continue; //Buffer o PSO no válido para el objeto
                }

                if (visible->isVisible == false) {
                    continue; //No es visible
                }

                vec4 viewPOS = viewMatrix * transform->getWorldMatrix().data[3];
                float depth = std::abs(viewPOS.z);

                uint64_t key = GenerateSortKey(staticMaterial->pso.lock(), depth, staticMaterial->runtimeMaterialID, 0, staticMesh->runtimeMeshID);

                renderItems.push_back({entity, key});
            }*/

            const mat4& viewMatrix = activeCamera->viewMatrix;

            size_t numberOfThreads = p_jobSystem->AvailableThreads();
            //Preparar la lista de las render entities comprobando que esten asignado los componentes necesarios para dibujar.
            {
                std::vector<Entity> entityList(entities.begin(), entities.end());
                size_t totalEntities = entityList.size();
                if(totalEntities == 0) return;

                size_t chunkSize = (totalEntities + numberOfThreads - 1) / numberOfThreads;
                size_t numChunks = (totalEntities + chunkSize - 1) / chunkSize; 

                lynx::job_system::JobGroup organizationalGroup;

                std::vector<std::vector<RenderItem>> perThreadRenderItems(numChunks);

                for(size_t i = 0; i < numChunks; i++){
                    size_t start = i * chunkSize;
                    size_t end = std::min(start + chunkSize, totalEntities);
                    perThreadRenderItems[i].reserve(chunkSize);

                    p_jobSystem->SubmitJob([this, start, end, entityList, transformComponentArray, staticMeshArray, staticMaterialArray, visibleArray, i, &perThreadRenderItems, viewMatrix, staticTextureArray](){
                        for(size_t j = start; j < end; j++){
                            Entity entity = entityList[j];
                            auto* transform = transformComponentArray->getData(entity);
                            auto* staticMesh = staticMeshArray->getData(entity);
                            auto* staticMaterial = staticMaterialArray->getData(entity);
                            auto* visible = visibleArray->getData(entity);
                            auto* texture = staticTextureArray->getData(entity);

                            if (!transform || !staticMesh || !staticMaterial || !visible || !texture) {
                                continue; //No cumple los requisitos para poder dibujarse en pantalla
                            }

                            if (visible->isVisible == false) {
                                continue; //No es visible
                            }

                            if (staticMesh->staticBuffers.lock() == nullptr || staticMaterial->pso.lock() == nullptr || texture->staticTexture.lock() == nullptr) {
                                continue; //Buffer o PSO no válido para el objeto
                            }

                            vec4 viewPOS = viewMatrix * transform->localToWorldMatrix.data[3];
                            float depth = std::abs(viewPOS.z);

                            uint64_t key = GenerateSortKey(staticMaterial->pso.lock(), depth, staticMaterial->runtimeMaterialID, texture->runtimeTexID, 0, staticMesh->runtimeMeshID);

                            perThreadRenderItems[i].push_back({entity, key});
                        }
                    }, &organizationalGroup);
                }

                organizationalGroup.Wait();
                for(auto vector: perThreadRenderItems){
                    renderItems.insert_range(renderItems.end(), std::move(vector));
                }
            }
            

            if (renderItems.empty()) {
                p_Renderer->returnParallelCommandEncoder(std::move(encoder));
                return;
            }

            //Ordenar la lista de dibujado
            std::sort(renderItems.begin(), renderItems.end());

            lastRenderItems = renderItems; //Así podemos acceder a la lista lista para dibujar en otras funciones que lo necesiten sin tener que recalcular

            //TODO: Implementar soporte para Instanced Rendering, y Frames in flight por parte del Renderer.
            encoder->SetGlobalBuffers(slots::GlobalBufferSlot_Camera, activeCamera->cameraBuffer.get());

            size_t totalOfItem = renderItems.size();
            size_t chunkSize = (totalOfItem + numberOfThreads - 1) / numberOfThreads;
            size_t numChunks = (totalOfItem + chunkSize - 1) / chunkSize; 

            //OLD
            //std::vector<std::future<void>> futures;

            lynx::job_system::JobGroup group;
            
            for (size_t i = 0; i < numChunks; i++) {
                size_t chunckStart = i * chunkSize;
                size_t chunckEnd = std::min(chunckStart + chunkSize, totalOfItem);

                p_jobSystem->SubmitJob( [this, encoderPtr, &renderItems, chunckStart, chunckEnd, instanceBuffer, staticMeshArray, staticMaterialArray, transformComponentArray, staticTextureArray]() {
                    //Cache para el batching
                    IGPURenderPipeline* lastPSO = nullptr;
                    uint16_t lastMaterialID = 0;
                    uint16_t lastTextureID = 0;
                    uint16_t lastMeshID = 0;

                    auto cmd = encoderPtr->GetThreadEncoder();

                    //std::vector<mat4> matrixVector;
                    bool isIntanceBufferSet = false;

                    size_t current = chunckStart;
                    while (current < chunckEnd) {
                        const auto& item = renderItems[current];
                        const uint64_t key = item.sortKey;

                        size_t batchEnd = current + 1;
                        while (batchEnd < chunckEnd && renderItems[batchEnd].sortKey == key) {
                            batchEnd++;
                        }

                        auto entity = item.entity;
                        //auto* transform = p_Coordinator->getComponent<TransformComponent>(entity); No tiene sentido obtener el transform, porque cada entidad tiene uno diferente. Mejor pedirlo al renderizar con logica personalizada
                        //segun el tipo de rendering
                        //auto* staticMesh = p_coordinator->getComponent<StaticRenderingData>(entity);
                        ///auto* staticMaterial = p_coordinator->getComponent<StaticMaterialRenderingData>(entity);
                        auto* staticMesh = staticMeshArray->getData(entity);
                        auto* staticMaterial = staticMaterialArray->getData(entity);
                        auto* staticTexture = staticTextureArray->getData(entity);

                        size_t totalOfInstance = batchEnd - current;
                        auto pso =  staticMaterial->pso.lock().get();
                        auto materialID = staticMaterial->runtimeMaterialID;
                        auto textureID = staticTexture->runtimeTexID;
                        auto meshID = staticMesh->runtimeMeshID;

                        if (pso != lastPSO) {
                            cmd->SetPipelineState(pso);
                            lastPSO = pso;
                        }

                        if (materialID != lastMaterialID || textureID != lastTextureID) {
                            //Vincular la _MainTex, que es obligatoria
                            auto texLink = staticTexture->staticTexture.lock();
                            if(texLink && texLink->texture && texLink->sampler){
                                cmd->SetFragmentTexture(0, texLink->texture.get());
                                cmd->SetFragmentSampler(0, texLink->sampler.get());
                            }

                            //Aquie iria la vinculacion con el buffer con el resto de elementos que requira el material.
                            lastTextureID = textureID;
                            lastMaterialID = materialID;
                        }
                        auto* meshPtr = staticMesh->staticBuffers.lock().get();

                        if (meshID != lastMeshID) {
                            cmd->SetVertexBuffer(slots::RenderBufferSlot_VertexMain, meshPtr->vertexBuffer.get());
                            cmd->SetIndexBuffer(meshPtr->indexBuffer.get());
                            lastMeshID = meshID;
                        }

                        if (totalOfInstance > m_minInstanceRenderingElements) {
                            //Instancing rendering
                            //LYNX_LOG_P(DEBUG_PREFIX, "Doing and instancing rendering dor %i elements", totalOfInstance);
                            /*if (matrixVector.size() < totalOfInstance) {
                                matrixVector.resize(totalOfInstance);
                            }*/

                            std::vector<structs::InstanceData> instanceDataVector(totalOfInstance);

                            for (size_t i = 0; i < totalOfInstance; i++) {
                                //auto* transform = p_coordinator->getComponent<TransformComponent>(renderItems[current + i].entity);
                                auto actualEntity = renderItems[current + i].entity;
                                auto* transform = transformComponentArray->getData(actualEntity);
                                auto* texture = staticTextureArray->getData(actualEntity);
                                structs::InstanceData instance;
                                instance.modelMatrix = transform->localToWorldMatrix;

                                vec4 finalUV{0.0f};

                                auto texLink = texture->staticTexture.lock();
                                if(texLink && texLink->atlasSprites.size() > texture->currentSpriteID){
                                    //El id de sprite es menor que el tamaño del atlas del sprite. Es un índice válido
                                    finalUV = texLink->atlasSprites[texture->currentSpriteID];

                                }else{
                                    LYNX_ERROR_P(DEBUG_PREFIX, "Error con la textura %s, el ID %i no existe en el atlas", texLink->texture->getDescriptor().label.c_str(), texture->currentSpriteID);
                                    finalUV = texLink->atlasSprites[0]; //Por seguridad.
                                }
                                //LYNX_LOG_P(DEBUG_PREFIX, "Datos del uv: %s", lynx::maths::vector::to_string(instance.uvRect).c_str());
                                //matrixVector[i] = transform->localToWorldMatrix;

                                //Ajustando flip del sprite.
                                if(texture->flipX){
                                    finalUV.x = finalUV.x + finalUV.z;
                                    finalUV.z = -finalUV.z;
                                }

                                if(texture->flipY){
                                    finalUV.y = finalUV.y + finalUV.w;
                                    finalUV.w = -finalUV.w;
                                }

                                instance.uvRect = finalUV;

                                instanceDataVector[i] = instance;
                            }

                            uint32_t offset = instanceBuffer->UploadInstanceData(instanceDataVector.data(), sizeof(structs::InstanceData) * totalOfInstance);

                            if (!isIntanceBufferSet) {
                                cmd->SetInstancedBuffer(slots::RenderBufferSlot_InstanceModel, instanceBuffer->GetBuffer());
                                isIntanceBufferSet = true;
                            }
                            auto indexCount = meshPtr->indexCount;

                            cmd->DrawIndexedInstanced(indexCount, totalOfInstance, offset, sizeof(structs::InstanceData));
                        }else {
                            //LYNX_WARNING_P(DEBUG_PREFIX, "Doing a normal draw using the indexed buffer");
                            //Render en bucle
                            if (!isIntanceBufferSet) {
                                cmd->SetInstancedBuffer(slots::RenderBufferSlot_InstanceModel, instanceBuffer->GetBuffer());
                                isIntanceBufferSet = true;
                            }

                            for (size_t i = 0; i < totalOfInstance; i++) {
                                //auto* transform = p_coordinator->getComponent<TransformComponent>(renderItems[current + i].entity);
                                auto* transform = transformComponentArray->getData(renderItems[current + i].entity);
                                auto* texture = staticTextureArray->getData(renderItems[current + i].entity);

                               // auto worldMatrix = transform->localToWorldMatrix;

                                structs::InstanceData instance;
                                instance.modelMatrix = transform->localToWorldMatrix;
                                vec4 finalUV{0.0f};

                                auto texLink = texture->staticTexture.lock();
                                if(texLink && texLink->atlasSprites.size() > texture->currentSpriteID){
                                    //El id de sprite es menor que el tamaño del atlas del sprite. Es un índice válido
                                    finalUV = texLink->atlasSprites[texture->currentSpriteID];
                                }else{
                                    LYNX_ERROR_P(DEBUG_PREFIX, "Error con la textura %s, el ID %i no existe en el atlas", texLink->texture->getDescriptor().label.c_str(), texture->currentSpriteID);
                                    finalUV = texLink->atlasSprites[0]; //Por seguridad.
                                }

                                //Ajustando flip del sprite.
                                if(texture->flipX){
                                    finalUV.x = finalUV.x + finalUV.z;
                                    finalUV.z = -finalUV.z;
                                }

                                if(texture->flipY){
                                    finalUV.y = finalUV.y + finalUV.w;
                                    finalUV.w = -finalUV.w;
                                }
                                
                                instance.uvRect = finalUV;

                                //LYNX_LOG_P(DEBUG_PREFIX, "Datos del uv: %s, en textura %s", lynx::maths::vector::to_string(instance.uvRect).c_str(), texLink->texture->getDescriptor().label.c_str());
                                //LYNX_LOG_P(DEBUG_PREFIX, "Tamaño del struct %i", sizeof(structs::InstanceData));

                                uint32_t offset = instanceBuffer->UploadInstanceData(&instance, sizeof(structs::InstanceData));
                                auto indexCount = meshPtr->indexCount;

                                cmd->DrawIndexedInstanced(indexCount, 1, offset, 256);
                            }
                        }

                        current = batchEnd;
                    }

                    cmd->EndEncoding();
                }, &group);
            }

            //Esperamos a que se codifiquen todas las llamadas
            /*for (auto& future : futures) {
                future.wait();
            }*/

            group.Wait();

            /*if (core::options::RenderingOptions::isPostProcessEnabled) {
                //Finalizar encoder, liberar, y asignarlo otro puntero. TODO
                IRenderer::FinalizeEncoding(encoderPtr);
            }else {
            }*/ //Post procesado o no, este Render pass para eficiencia, lo puede reutilizar el sistema de post_procesado. Sino, ya el render normal lo termina :)


            //El End encoding lo llama el hilo principal.
        }
        
        p_Renderer->returnParallelCommandEncoder(std::move(encoder));
    }

    uint64_t RenderDrawSystem::GenerateSortKey(const std::shared_ptr<IGPURenderPipeline> &pso, float depth, uint16_t materialRuntimeID, uint16_t textureRuntimeID, uint8_t drawIndex, uint16_t meshID) {
        const auto queue = pso->getRenderQueue();
        const auto psoID = pso->getID();
        
        uint64_t finalKey = 0;

        // Queue (8 bits): Siempre a la izquierda para separar los esquemas de dibujado
        finalKey |= (static_cast<uint64_t>(queue) & 0xFF) << 56; 

        if (queue <= LYNX_GPU_RENDER_QUEUE_TYPE::LYNX_GPU_RENDER_QUEUE_ALPHA_TEST) {
            // ---------------------------------------------------------------------
            // OBJETOS OPACOS
            // Ordenados por costo de hardware: PSO > Textura > Material > Mesh
            // ---------------------------------------------------------------------

            // PSO: 12 bits (Máximo 4,096 shaders en el juego)
            finalKey |= (static_cast<uint64_t>(psoID) & 0xFFF) << 44;

            // Textura: 14 bits (Máximo 16,384 texturas en memoria)
            finalKey |= (static_cast<uint64_t>(textureRuntimeID) & 0x3FFF) << 30;

            // Material: 14 bits (Máximo 16,384 instancias de material)
            finalKey |= (static_cast<uint64_t>(materialRuntimeID) & 0x3FFF) << 16;

            // Mesh: 12 bits (Máximo 4,096 modelos 3D distintos cargados a la vez)
            finalKey |= (static_cast<uint64_t>(meshID) & 0xFFF) << 4;

            // DrawIndex: 4 bits (Máximo 16 sub-meshes por modelo)
            finalKey |= (static_cast<uint64_t>(drawIndex) & 0xF);

        } else {
            // ---------------------------------------------------------------------
            // OBJETOS TRANSPARENTES
            // El orden Z es el rey. Ordenados de atrás hacia adelante.
            // ---------------------------------------------------------------------
            
            float clampedDepth = std::min(depth, 655.35f);
            auto depthValue = static_cast<uint64_t>(clampedDepth * 100) & 0xFFFF;
            auto invertedDepthKey = 0xFFFF - depthValue;

            // Profundidad Invertida: 16 bits
            finalKey |= (invertedDepthKey & 0xFFFF) << 40;

            // PSO: 12 bits
            finalKey |= (static_cast<uint64_t>(psoID) & 0xFFF) << 28;

            // Textura: 14 bits
            finalKey |= (static_cast<uint64_t>(textureRuntimeID) & 0x3FFF) << 14;

            // Material: 10 bits (Máximo 1,024 materiales transparentes a la vez)
            finalKey |= (static_cast<uint64_t>(materialRuntimeID) & 0x3FF) << 4;
            
            // DrawIndex: 4 bits
            finalKey |= (static_cast<uint64_t>(drawIndex) & 0xF);
            
            // Nota: En objetos transparentes, la profundidad rompe casi todo el batching, 
            // por lo que el MeshID se omite para dar espacio a los demás identificadores.
        }

        return finalKey;
    }
}
