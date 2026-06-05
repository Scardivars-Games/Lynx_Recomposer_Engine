#include <ilen_public/API/scenes_api.h>
#include <ilen/core/engine_context.h>
#include <ilen/resources/scene_manager.h>

using namespace ilen::engine::api;

void ScenesAPI::Init(const ilen::EngineContext* context){
    p_sceneManager = context->p_sceneManager.get();
}

ilen::resources::scene::AsyncOperationPtr ScenesAPI::LoadSceneAdditiveAsync(const std::string& sceneName) {
    return p_sceneManager->LoadSceneAsync(sceneName, ilen::resources::scene::LoadSceneMode::Additive);
}