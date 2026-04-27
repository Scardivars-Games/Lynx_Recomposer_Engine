#include <ilen/render/post-process-effects/post-process-system.h>
#include <ilen/core/runtime_options.h>
#include <ilen/core/engine-context.h>
#include <ilen/render/gpu-interfaces/parallel-command-encoder.h>
#include <ilen/render/render-draw-system.h>

using namespace ilen::ecs::systems;
using namespace ilen::maths::matrix;


void PostProcessSystem::Init(const EngineContext *engineContext) {
    coordinator = engineContext->p_coordinator.get();
    renderer = engineContext->p_gameRenderer.get();
    shaderManager = engineContext->p_shaderManager.get();
    meshManager = engineContext->p_meshManager.get();
    //drawSystem = engineContext->p_renderDrawSystem.get();
    //cameraSystem = engineContext->p_cameraSystem.get();
    jobSystem = engineContext->p_jobSystem.get();
    drawSystem = coordinator->getSystem<RenderDrawSystem>().get();
    cameraSystem = coordinator->getSystem<CameraSystem>().get();


    //Todo, crear los PSO especiales para el post-procesado, según esten en uso o no. (Completado, creo)
    //De momento, creo que un unico PSO para postprocesado debe ser suficiente.
    auto postProcessPSO = GenerateNewPSO();

    ilen::render::IlenParallelEncoderDescriptor temporalDescriptor {};
    postProcessPSO[0].encoderDescriptor = &temporalDescriptor; // Inicia el motor, no se sabe que post procesados hay en memoria.

    encoderToPostProcessPSO.second = renderer->createRenderPipeline(postProcessPSO[0], shaderManager);
    encoderToPostProcessPSO.first = std::move(temporalDescriptor);

    postProcessPresentPSO = renderer->createRenderPipeline(postProcessPSO[1], shaderManager);

    quadGPUMesh = meshManager->getMeshHandlers(resources::QuadMeshID);

    size_t instanceBufferSize = 4 * 1024 * 1024; //4 MB (con eso es suficiente para pruebas y así no me asusto tanto por el brindo de RAM. A futuro cambiar a 16)
    auto framesInFlight = renderer->getActualNumOfFramesInFlight();
    postProcessInstanceBuffer.resize(framesInFlight);
    for (int i = 0; i < framesInFlight; i++) {
        postProcessInstanceBuffer[i] = renderer->createInstanceBuffer(instanceBufferSize, "Instance Buffer For Post-Processing");
    }
}

void PostProcessSystem::Update() {
    auto quadPtr = quadGPUMesh.lock();
    if (quadPtr == nullptr) {
        meshManager->UpdateProcess();
        quadGPUMesh = meshManager->getMeshHandlers(resources::QuadMeshID);
        ILEN_LOG_P(DEBUG_PREFIX, "Saltando el frame porque no esta en memoria el mesh para el Quad.");
        return; //Saltamos el frame
    }

    if (ilen::core::options::GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS) {
        //Comprobar si el Parallel es el mismo que esta configurado en el PSO!!!
        auto actualFrameInFlight = renderer->getActualFrameInFlight();
        postProcessInstanceBuffer[actualFrameInFlight]->OnBeginFrame();
        auto actualParallel = renderer->getParallelCommandEncoder();
        auto postProcessPSOs = GenerateNewPSO();
        bool changedPSO = false;

        if (*actualParallel->getParallelEncoderDescriptor() != encoderToPostProcessPSO.first) {
            ILEN_LOG_P(DEBUG_PREFIX, "Generating new PSO for post-processing. Reason: Encoder Descriptor changed");
            auto *descriptor = actualParallel->getParallelEncoderDescriptor(); //Return a *
            postProcessPSOs[0].encoderDescriptor = descriptor;

            /*auto descriptorToRender = *descriptor;
            descriptorToRender.colorAttachments.resize(1); //Eliminamos el resto
            postProcessPSOs[1].encoderDescriptor = &descriptorToRender;*/

            encoderToPostProcessPSO.second = renderer->createRenderPipeline(postProcessPSOs[0], shaderManager);
            encoderToPostProcessPSO.first = *descriptor;
            changedPSO = true;

            //postProcessPresentPSO = renderer->createRenderPipeline(postProcessPSOs[1], shaderManager);
        }
        //Post processing code
        //Obtenemos la lista de todo lo que si dibujo anteriormente. La lista ya viene sorteada.
        auto renderListItems = drawSystem->getLastUsedRenderItemList();
        auto encoderPtr = actualParallel.get();

        //Instance buffer donde escribir.
        auto* instanceBuffer = postProcessInstanceBuffer[actualFrameInFlight].get();
        if (instanceBuffer == nullptr) {
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Instance buffer incorrecto en el área de Post Procesado. Revisar el proceso de creación.");
        }

        size_t totalOfItem = renderListItems.size();
        size_t numberOfThreads = jobSystem->AvailableThreads();
        size_t itemsPerThread = (totalOfItem + numberOfThreads - 1) / numberOfThreads;

        if (previousTransforms.size() != totalOfItem) {
            previousTransforms.reserve(totalOfItem);
        }

        auto activeCamera = cameraSystem->getMainCamera();
        if (!activeCamera) {
            ILEN_WARNING_P(DEBUG_PREFIX, "No se encontró una cámara activa, por lo que no se puede dibujar.");
            return; //Sin camara, no puedo dibujar.
        }

        std::unordered_map<Entity, maths::matrix::mat4> previousFrameMatrices;

        {
            std::lock_guard<std::mutex> lock(previousTransformMutex);
            previousFrameMatrices = previousTransforms;
        }

        //OLD
        //std::vector<std::future<void>> futures;

        //new
        ilen::job_system::JobGroup group;

        encoderPtr->SetGlobalBuffers(slots::GlobalBufferSlot_Camera, activeCamera->getViewProjectionCameraBuffer());

        for (size_t i = 0; i < numberOfThreads; i++) {
            size_t chunckStart = i * itemsPerThread;
            size_t chunckEnd = std::min(chunckStart + itemsPerThread, totalOfItem);

            if (chunckStart >= chunckEnd) continue;

            jobSystem->SubmitJob( [this, encoderPtr, &renderListItems, chunckStart, chunckEnd, instanceBuffer, &previousFrameMatrices]() {
                //Cache para el batching
                IGPURenderPipeline* lastPSO = nullptr;
                uint16_t lastMaterialID = 0;
                uint16_t lastMeshID = 0;

                auto cmd = encoderPtr->GetThreadEncoder();

                std::vector<structs::InstanceDataForVelocityBuffer> instanceBufferData_PostProcess;
                bool isIntanceBufferSet = false;

                size_t current = chunckStart;
                while (current < chunckEnd) {
                    const auto& item = renderListItems[current];
                    const uint64_t key = item.sortKey;

                    size_t batchEnd = current + 1;
                    while (batchEnd < chunckEnd && renderListItems[batchEnd].sortKey == key) {
                        batchEnd++;
                    }

                    auto entity = item.entity;
                    //auto* transform = p_Coordinator->getComponent<TransformComponent>(entity); No tiene sentido obtener el transform, porque cada entidad tiene uno diferente. Mejor pedirlo al renderizar con logica personalizada
                    //segun el tipo de rendering
                    auto* staticMesh = coordinator->getComponent<components::StaticRenderingData>(entity);

                    size_t totalOfInstance = batchEnd - current;
                    auto meshID = staticMesh->runtimeMeshID;

                    //Solo hay un único PSO en este proceso de cálculos.
                    cmd->SetPipelineState(encoderToPostProcessPSO.second.get());

                    auto* meshPtr = staticMesh->staticBuffers.lock().get();

                    if (meshID != lastMeshID) {
                        cmd->SetVertexBuffer(slots::RenderBufferSlot_VertexMain, meshPtr->vertexBuffer.get());
                        cmd->SetIndexBuffer(meshPtr->indexBuffer.get());
                        lastMeshID = meshID;
                    }

                    if (totalOfInstance > 5) {
                        //Instancing rendering
                        //ILEN_LOG_P(DEBUG_PREFIX, "Doing and instancing rendering dor %i elements", totalOfInstance);
                        if (instanceBufferData_PostProcess.size() < totalOfInstance) {
                            instanceBufferData_PostProcess.resize(totalOfInstance);
                        }

                        /*for (size_t i = 0; i < totalOfInstance; i++) {
                            auto* transform = coordinator->getComponent<components::TransformComponent>(renderListItems[current + i].entity);
                            auto [it, inserted] = previousTransforms.try_emplace(renderListItems[current + i].entity, *transform);

                            instanceBufferData_PostProcess[i].actualModelMatrix = transform->getWorldMatrix();
                            instanceBufferData_PostProcess[i].previousModelMatrix = it->second.getWorldMatrix();
                        }*/

                        for(size_t i = 0; i < totalOfInstance; i++){
                            auto entity = renderListItems[current + i].entity;
                            auto* transform = coordinator->getComponent<components::TransformComponent>(entity);
                            auto currentMatrix = transform->getWorldMatrix();

                            //Buscamos la matriz del frame anterios SOLO LEERLA
                            auto prevIt = previousFrameMatrices.find(entity);
                            maths::matrix::mat4 previousMatrix = (prevIt != previousFrameMatrices.end())
                                ? prevIt->second
                                : currentMatrix; //Si es el primer frame, usar el mismo.

                            instanceBufferData_PostProcess[i].actualModelMatrix = currentMatrix;
                            instanceBufferData_PostProcess[i].previousModelMatrix = previousMatrix;
                        }

                        uint32_t offset = instanceBuffer->UploadInstanceData(instanceBufferData_PostProcess.data(), sizeof(structs::InstanceDataForVelocityBuffer) * totalOfInstance);

                        if (!isIntanceBufferSet) {
                            cmd->SetInstancedBuffer(slots::RenderBufferSlot_InstanceModel, instanceBuffer->GetBuffer());
                            isIntanceBufferSet = true;
                        }
                        auto indexCount = meshPtr->indexCount;

                        cmd->DrawIndexedInstanced(indexCount, totalOfInstance, offset, sizeof(structs::InstanceDataForVelocityBuffer));
                    }else {
                        //ILEN_WARNING_P(DEBUG_PREFIX, "Doing a normal draw using the indexed buffer");
                        //Render en bucle
                        if (!isIntanceBufferSet) {
                            cmd->SetInstancedBuffer(slots::RenderBufferSlot_InstanceModel, instanceBuffer->GetBuffer());
                            isIntanceBufferSet = true;
                        }

                        for (size_t i = 0; i < totalOfInstance; i++) {
                            auto entity = renderListItems[current + i].entity;
                            auto* transform = coordinator->getComponent<components::TransformComponent>(entity);
                            structs::InstanceDataForVelocityBuffer instanceBufferData{};
                            auto currentMatrix = transform->getWorldMatrix();

                            auto prevIt = previousFrameMatrices.find(entity);
                            maths::matrix::mat4 previousMatrix = (prevIt != previousFrameMatrices.end())
                                ? prevIt->second
                                : currentMatrix; // primer frame de vida: sin blur es correcto

                            instanceBufferData.actualModelMatrix = currentMatrix;
                            instanceBufferData.previousModelMatrix = previousMatrix;
                            uint32_t offset = instanceBuffer->UploadInstanceData(&instanceBufferData, sizeof(structs::InstanceDataForVelocityBuffer));
                            auto indexCount = meshPtr->indexCount;

                            cmd->DrawIndexedInstanced(indexCount, 1, offset, sizeof(structs::InstanceDataForVelocityBuffer));
                        }
                    }

                    current = batchEnd;
                }

                cmd->EndEncoding();
            }, &group);
        }

        //Esperamos a que se codifiquen todas las llamadas
        //OLD
        /*for (auto& future : futures) {
            future.wait();
        }*/

        //New
        group.Wait();

        {
            std::lock_guard<std::mutex> lock(previousTransformMutex);
            previousTransforms.clear();

            for(const auto& item: renderListItems){
                auto* transform = coordinator->getComponent<components::TransformComponent>(item.entity);
                if(transform){
                    previousTransforms[item.entity] = transform->getWorldMatrix();
                }
            }
        }

        //Finalizando el render actual de calculos, y dibujado, y pasamos a crear un nuevo para unir los efectos.
        renderer->endEncoding(actualParallel.get()); //Finalizamos el render actual, y generar uno nuevo.
        IlenParallelEncoderDescriptor renderDescriptor {};
        renderDescriptor.colorAttachments.resize(1); //Solo posee 1 (El drawable.)
        renderDescriptor.label = "Post-Process Parallel Renderer";
        auto nativeTexture = renderer->getAllSceneTexturesPtr(); //Esto es un std::Vector con las texturas usadas en el color attachment

        auto drawableAttach = std::make_unique<IlenEncoderAttachmentDescriptor>();
        auto imagesAttach = renderer->getAllSceneTexturesPtr(); //Aqui solo llegan las texturas del Frame actual !!!

        drawableAttach->texture = renderer->getDrawableTexturePtr();
        drawableAttach->loadOption = ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_CLEAR;
        drawableAttach->clearColor = color::Black;
        drawableAttach->storeOption = ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_STORE;

        renderDescriptor.colorAttachments[0] = std::move(drawableAttach);

        if (changedPSO) {
            postProcessPSOs[1].encoderDescriptor = &renderDescriptor;
            postProcessPresentPSO = renderer->createRenderPipeline(postProcessPSOs[1], shaderManager);
        }

        auto newParallelEncoder = renderer->createNewParallelCommandEncoder(std::move(renderDescriptor));

        auto encoder = newParallelEncoder->GetThreadEncoder();//Solo se necesita un encoder, ya que solo se va a dibujar una vez.

        //Asignando todo lo que requiere el encoder para funcionar en este caso.
        encoder->SetPipelineState(postProcessPresentPSO.get());
        encoder->SetVertexBuffer(slots::RenderBufferSlot_VertexMain, quadPtr->vertexBuffer.get());
        encoder->SetIndexBuffer(quadPtr->indexBuffer.get());
        encoder->SetTexture(0, nativeTexture[0]); //Esto siempre será la textura base de color con los datos renderizados.
        size_t actualImageIndex = 1;
        if (core::options::GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_MOTION_BLUR) {
            encoder->SetTexture(slots::TextureBufferSlot_VelocityBuffer, imagesAttach[actualImageIndex]);
        }
        encoder->DrawIndexed(quadPtr->indexCount);
        //TODO: Actualmente no dibuja nada en pantalla. Posible causa: el PSO tiene prohibido escribir en el buffer 0 (textura de color en nuestro caso. Revisar)
        encoder->EndEncoding();
        renderer->returnParallelCommandEncoder(std::move(newParallelEncoder));
    }
}

std::vector<IlenRenderPipelineDescriptor> PostProcessSystem::GenerateNewPSO() {
    std::vector<IlenRenderPipelineDescriptor> toReturn;
    auto actualGraphicsOptions = core::options::GraphicsOptions::graphicsOptionsFlags;
    std::vector<IlenConstantValuesForPipeline> constants;

    if (actualGraphicsOptions & ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS) {
        //Todos los bools de post procesado se definen aquí, y su valor depende del resultado del bitwise.
        IlenConstantValuesForPipeline valueForMotionBlur {};
        valueForMotionBlur.actualValue = actualGraphicsOptions & ILEN_GRAPHICS_OPTIONS_USE_MOTION_BLUR;
        valueForMotionBlur.index = slots::TextureBufferSlot_VelocityBuffer;

        //Subimos cada valor.
        constants.push_back(valueForMotionBlur);
    }
    
    

    //TODO: Las funciones y los id de shader no son los correctos en este caso. Ajustar en el futuro
    IlenMaterial postProcessMat{};
    postProcessMat.fragmentFunctionName = "post_processing_calculations_fragment";
    postProcessMat.vertexFunctionName = "post_processing_calculations_vertex";
    postProcessMat.vertexShaderID = resources::PostProcessCalculateShaderID;
    postProcessMat.fragmentShaderID = resources::PostProcessCalculateShaderID;
    postProcessMat.materialName = "Post Processing - Calculating Values";
    postProcessMat.renderQueue = ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE;

    //Nota importante, si esto se esta ejecutando es porque estan activados los efectos de postProcesado.
    IlenRenderPipelineDescriptor postProcessPSO {postProcessMat};
    postProcessPSO.isSpecialPipeline = true;
    postProcessPSO.constantValuesForPipelines = constants;

    toReturn.push_back(postProcessPSO);


    IlenMaterial postProcessRenderMat{};
    postProcessMat.fragmentFunctionName = "post_processing_fragment";
    postProcessMat.vertexFunctionName = "post_processing_vertex";
    postProcessMat.vertexShaderID = resources::PostProcessPresentShaderID;
    postProcessMat.fragmentShaderID = resources::PostProcessPresentShaderID;
    postProcessMat.materialName = "Post Processing - Final Rendering";
    postProcessMat.renderQueue = ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE;

    //Nota importante, si esto se esta ejecutando es porque estan activados los efectos de postProcesado.
    IlenRenderPipelineDescriptor postProcessRenderPSO {postProcessMat};
    postProcessRenderPSO.constantValuesForPipelines = constants;

    toReturn.push_back(postProcessRenderPSO);
    return toReturn;
}



