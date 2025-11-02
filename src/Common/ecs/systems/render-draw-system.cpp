#include <ilen/render/render-draw-system.h>


namespace ilen::ecs::systems {
    void RenderDrawSystem::Init(const EngineContext* engineContext) {
        p_Coordinator = engineContext->p_coordinator.get();
        p_JobSystem = engineContext->p_jobSystem.get();
        p_Renderer = engineContext->p_gameRenderer.get();
    }

    void RenderDrawSystem::Update() {
        //Obtener camara activa
        auto activeCamera = p_cameraSystem->getMainCamera();


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