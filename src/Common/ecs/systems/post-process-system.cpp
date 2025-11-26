#include <ilen/render/post-process-effects/post-process-system.h>
#include <ilen/core/runtime_options.h>

using namespace ilen::ecs::systems;

void PostProcessSystem::Init(const EngineContext *engineContext) {
    coordinator = engineContext->p_coordinator.get();
    renderer = engineContext->p_gameRenderer.get();
    //Todo, crear los PSO especiales para el post-procesado, según esten en uso o no.
}

void PostProcessSystem::Update() {
    if (ilen::core::options::RenderingOptions::isPostProcessEnabled) {
        //Post processing code
        for (auto const& entity: entities) {

        }
    }
}

