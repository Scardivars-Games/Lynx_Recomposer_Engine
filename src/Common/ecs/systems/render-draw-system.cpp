#include <ilen/render/render-draw-system.h>
#include <ilen/render/render-constants.h>
#include <ilen/core/runtime_options.h>
#include <ilen/transform/transform-component.h>
#include <ilen/core/engine-context.h>
#include <ilen/render/renderer.h>
#include <ilen/render/render-component.h>
#include <ilen/camera/camera-system.h>

namespace ilen::ecs::systems {

    using namespace ilen::maths::matrix;
    using namespace ilen::ecs::components;

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
            ilen::time::ScopedTimer timer("Time to get _drawable in Metal");
            //Obtener el encoder (paraleliza el trabajo)
            encoder = p_Renderer->getParallelCommandEncoder();
        }
        if (!encoder) {
            return; //No hay un encoder al que llamar para dibujar
        }
        auto encoderPtr = encoder.get();


        {
            ilen::time::ScopedTimer timer("Opaque screen rendering");
            auto* instanceBuffer = p_Renderer->getInstanceBuffer();
            if (instanceBuffer == nullptr) {
                ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Renderer devolvio un Instance Buffer incorrecto. La aplicación no puede continuar su ejecución");
            }

            //Obtener camara activa
            auto activeCamera = p_cameraSystem->getMainCamera();
            if (!activeCamera) {
                ILEN_WARNING_P(DEBUG_PREFIX, "No se encontró una cámara activa, por lo que no se puede dibujar.");
                p_Renderer->returnParallelCommandEncoder(std::move(encoder));
                return; //Sin camara, no puedo dibujar.
            }

            //Obtener punteros a los array de los componentes que requiere el sistema para trabajar
            auto* transformComponentArray = p_coordinator->getComponentArray<TransformComponent>();
            auto* staticMeshArray = p_coordinator->getComponentArray<StaticRenderingData>();
            auto* staticMaterialArray = p_coordinator->getComponentArray<StaticMaterialRenderingData>();
            auto* visibleArray = p_coordinator->getComponentArray<VisibilityComponent>();

            if(!transformComponentArray || !staticMeshArray || !staticMaterialArray || !visibleArray){
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

            const mat4& viewMatrix = activeCamera->getViewMatrix();

            size_t numberOfThreads = p_jobSystem->AvailableThreads();
            //Preparar la lista de las render entities comprobando que esten asignado los componentes necesarios para dibujar.
            {
                std::vector<Entity> entityList(entities.begin(), entities.end());
                size_t totalEntities = entityList.size();
                if(totalEntities == 0) return;

                size_t chunkSize = (totalEntities + numberOfThreads - 1) / numberOfThreads;
                size_t numChunks = (totalEntities + chunkSize - 1) / chunkSize; 

                ilen::job_system::JobGroup organizationalGroup;

                std::vector<std::vector<RenderItem>> perThreadRenderItems(numChunks);

                for(size_t i = 0; i < numChunks; i++){
                    size_t start = i * chunkSize;
                    size_t end = std::min(start + chunkSize, totalEntities);
                    perThreadRenderItems[i].reserve(chunkSize);

                    p_jobSystem->SubmitJob([this, start, end, entityList, transformComponentArray, staticMeshArray, staticMaterialArray, visibleArray, i, &perThreadRenderItems, viewMatrix](){
                        for(size_t j = start; j < end; j++){
                            Entity entity = entityList[j];
                            auto* transform = transformComponentArray->getData(entity);
                            auto* staticMesh = staticMeshArray->getData(entity);
                            auto* staticMaterial = staticMaterialArray->getData(entity);
                            auto* visible = visibleArray->getData(entity);

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
            encoder->SetGlobalBuffers(slots::GlobalBufferSlot_Camera, activeCamera->getViewProjectionCameraBuffer());

            size_t totalOfItem = renderItems.size();
            size_t chunkSize = (totalOfItem + numberOfThreads - 1) / numberOfThreads;
            size_t numChunks = (totalOfItem + chunkSize - 1) / chunkSize; 

            //OLD
            //std::vector<std::future<void>> futures;

            ilen::job_system::JobGroup group;
            
            for (size_t i = 0; i < numChunks; i++) {
                size_t chunckStart = i * chunkSize;
                size_t chunckEnd = std::min(chunckStart + chunkSize, totalOfItem);

                p_jobSystem->SubmitJob( [this, encoderPtr, &renderItems, chunckStart, chunckEnd, instanceBuffer, staticMeshArray, staticMaterialArray, transformComponentArray]() {
                    //Cache para el batching
                    IGPURenderPipeline* lastPSO = nullptr;
                    uint16_t lastMaterialID = 0;
                    uint16_t lastMeshID = 0;

                    auto cmd = encoderPtr->GetThreadEncoder();

                    std::vector<mat4> matrixVector;
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

                        size_t totalOfInstance = batchEnd - current;
                        auto pso =  staticMaterial->pso.lock().get();
                        auto materialID = staticMaterial->runtimeMaterialID;
                        auto meshID = staticMesh->runtimeMeshID;

                        if (pso != lastPSO) {
                            cmd->SetPipelineState(pso);
                            lastPSO = pso;
                        }

                        if (materialID != lastMaterialID) {
                            //Aquí iria el binding con los Uniforms del material
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
                            //ILEN_LOG_P(DEBUG_PREFIX, "Doing and instancing rendering dor %i elements", totalOfInstance);
                            if (matrixVector.size() < totalOfInstance) {
                                matrixVector.resize(totalOfInstance);
                            }

                            for (size_t i = 0; i < totalOfInstance; i++) {
                                //auto* transform = p_coordinator->getComponent<TransformComponent>(renderItems[current + i].entity);
                                auto* transform = transformComponentArray->getData(renderItems[current + i].entity);
                                matrixVector[i] = transform->getWorldMatrix();
                            }

                            uint32_t offset = instanceBuffer->UploadInstanceData(matrixVector.data(), sizeof(mat4) * totalOfInstance);

                            if (!isIntanceBufferSet) {
                                cmd->SetInstancedBuffer(slots::RenderBufferSlot_InstanceModel, instanceBuffer->GetBuffer());
                                isIntanceBufferSet = true;
                            }
                            auto indexCount = meshPtr->indexCount;

                            cmd->DrawIndexedInstanced(indexCount, totalOfInstance, offset, sizeof(structs::InstanceData));
                        }else {
                            //ILEN_WARNING_P(DEBUG_PREFIX, "Doing a normal draw using the indexed buffer");
                            //Render en bucle
                            if (!isIntanceBufferSet) {
                                cmd->SetInstancedBuffer(slots::RenderBufferSlot_InstanceModel, instanceBuffer->GetBuffer());
                                isIntanceBufferSet = true;
                            }

                            for (size_t i = 0; i < totalOfInstance; i++) {
                                //auto* transform = p_coordinator->getComponent<TransformComponent>(renderItems[current + i].entity);
                                auto* transform = transformComponentArray->getData(renderItems[current + i].entity);
                                auto worldMatrix = transform->getWorldMatrix();
                                uint32_t offset = instanceBuffer->UploadInstanceData(worldMatrix.data, sizeof(mat4));
                                auto indexCount = meshPtr->indexCount;

                                cmd->DrawIndexedInstanced(indexCount, 1, offset, sizeof(structs::InstanceData));
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

    uint64_t RenderDrawSystem::GenerateSortKey(const std::shared_ptr<IGPURenderPipeline> &pso, float depth, uint16_t materialRuntimeID, uint8_t drawIndex, uint16_t meshID) {
        const auto queue = pso->getRenderQueue();
        const auto psoID = pso->getID();
        const auto queueKey = static_cast<uint64_t>(queue);

        auto drawKey = static_cast<uint64_t>(drawIndex) & 0xFF;

        uint64_t finalKey = 0;

        finalKey |= (queueKey << 56); //Bits más a la izquierda indican en que Schema se dibuja

        if (queue <= ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_ALPHA_TEST) {
            //Son objetos opacos

            finalKey |= ((uint64_t)psoID << 40); // Siguientes 16 asignan el ID del PSO en uso.
            finalKey |= ((uint64_t)materialRuntimeID << 24);
            finalKey |= ((uint64_t)meshID << 8);
            finalKey |= drawKey;
        } else {
            //SOn elementos transparentes. Importa más el orden en pantalla, que el PSO activo. Además se debe dibujar de atras hacia adelante.
            float clampedDepth = std::min(depth, 655.35f);
            auto depthValue = static_cast<uint64_t>(clampedDepth * 100) & 0xFFFF;
            auto invertedDepthKey = 0xFFFF - depthValue;

            finalKey |= (invertedDepthKey << 40);
            finalKey |= ((uint64_t)psoID << 24);
            finalKey |= ((uint64_t)materialRuntimeID << 8);
            finalKey |= drawKey;
        }

        return finalKey;
    }
}
