#include <ilen/render/render-draw-system.h>
#include <ilen/render/render-constants.h>
#include <ilen/transform/transform-component.h>


namespace ilen::ecs::systems {

    using namespace ilen::maths::matrix;
    using namespace ilen::ecs::components;

    void RenderDrawSystem::Init(const EngineContext* engineContext) {
        p_Coordinator = engineContext->p_coordinator.get();
        p_JobSystem = engineContext->p_jobSystem.get();
        p_Renderer = engineContext->p_gameRenderer.get();
        p_cameraSystem = engineContext->p_cameraSystem.get();
        p_matManager = engineContext->p_materialManager.get();
        p_meshManager = engineContext->p_meshManager.get();
    }

    void RenderDrawSystem::Update() {
        auto* instanceBuffer = p_Renderer->getInstanceBuffer();
        if (instanceBuffer == nullptr) {
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Renderer devolvio un Instance Buffer incorrecto. La aplicación no puede continuar su ejecución");
        }

        //Obtener camara activa
        auto activeCamera = p_cameraSystem->getMainCamera();
        if (!activeCamera) {
            ILEN_WARNING_P(DEBUG_PREFIX, "No se encontró una cámara activa, por lo que no se puede dibujar.");
            return; //Sin camara, no puedo dibujar.
        }

        //Obtener el encoder (paraleliza el trabajo)
        auto encoder = p_Renderer->getParallelCommandEncoder();
        if (!encoder) {
            return; //No hay un encoder al que llamar para dibujar
        }

        std::vector<RenderItem> renderItems;
        renderItems.reserve(entities.size());

        //ECS no manda entidades a este objeto. Problema de firmas, seguramente.
        const mat4& viewMatrix = activeCamera->getViewMatrix();
        for (auto const& entity: entities) {
            auto* transform = p_Coordinator->getComponent<TransformComponent>(entity);
            auto* staticMesh = p_Coordinator->getComponent<StaticRenderingData>(entity);
            auto* staticMaterial = p_Coordinator->getComponent<StaticMaterialRenderingData>(entity);
            auto* visible = p_Coordinator->getComponent<VisibilityComponent>(entity);

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
        }

        if (renderItems.empty()) {
            return;
        }

        //Ordenar la lista de dibujado
        std::sort(renderItems.begin(), renderItems.end());

        //TODO: Implementar soporte para Instanced Rendering, y Frames in flight por parte del Renderer.
        encoder->SetGlobalBuffers(slots::GlobalBufferSlot_Camera, activeCamera->getViewProjectionCameraBuffer());

        size_t totalOfItem = renderItems.size();
        size_t numberOfThreads = p_JobSystem->AvailableThreads();
        size_t itemsPerThread = (totalOfItem + numberOfThreads - 1) / numberOfThreads;

        std::vector<std::future<void>> futures;

        for (size_t i = 0; i < numberOfThreads; i++) {
            size_t chunckStart = i * itemsPerThread;
            size_t chunckEnd = std::min(chunckStart + itemsPerThread, totalOfItem);

            if (chunckStart >= chunckEnd) continue;

            futures.push_back(p_JobSystem->SubmitJob( [this, encoder, &renderItems, chunckStart, chunckEnd, instanceBuffer]() {
                //Cache para el batching
                IGPURenderPipeline* lastPSO = nullptr;
                uint16_t lastMaterialID = 0;
                uint16_t lastMeshID = 0;

                auto cmd = encoder->GetThreadEncoder();

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
                    auto* staticMesh = p_Coordinator->getComponent<StaticRenderingData>(entity);
                    auto* staticMaterial = p_Coordinator->getComponent<StaticMaterialRenderingData>(entity);

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
                        ILEN_LOG_P(DEBUG_PREFIX, "Doing and instancing rendering dor %i elements", totalOfInstance);
                        if (matrixVector.size() < totalOfInstance) {
                            matrixVector.resize(totalOfInstance);
                        }

                        for (size_t i = 0; i < totalOfInstance; i++) {
                            auto* transform = p_Coordinator->getComponent<TransformComponent>(renderItems[current + i].entity);
                            matrixVector[i] = transform->getWorldMatrix();
                        }

                        uint32_t offset = instanceBuffer->UploadInstanceData(matrixVector.data(), sizeof(mat4) * totalOfInstance);

                        if (!isIntanceBufferSet) {
                            cmd->SetInstancedBuffer(slots::RenderBufferSlot_InstanceModel, instanceBuffer->GetBuffer());
                            isIntanceBufferSet = true;
                        }
                        auto indexCount = meshPtr->indexCount;

                        cmd->DrawIndexedInstanced(indexCount, totalOfInstance, offset);
                    }else {
                        //ILEN_WARNING_P(DEBUG_PREFIX, "Doing a normal draw using the indexed buffer");
                        //Render en bucle
                        if (!isIntanceBufferSet) {
                            cmd->SetInstancedBuffer(slots::RenderBufferSlot_InstanceModel, instanceBuffer->GetBuffer());
                            isIntanceBufferSet = true;
                        }

                        for (size_t i = 0; i < totalOfInstance; i++) {
                            auto* transform = p_Coordinator->getComponent<TransformComponent>(renderItems[current + i].entity);
                            auto worldMatrix = transform->getWorldMatrix();
                            uint32_t offset = instanceBuffer->UploadInstanceData(worldMatrix.data, sizeof(mat4));
                            auto indexCount = meshPtr->indexCount;

                            cmd->DrawIndexedInstanced(indexCount, 1, offset);
                        }
                    }

                    current = batchEnd;
                }

                cmd->EndEncoding();
            }));
        }

        //Esperamos a que se codifiquen todas las llamadas
        for (auto& future : futures) {
            future.wait();
        }

        //El End encoding lo llama el hilo principal.
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
