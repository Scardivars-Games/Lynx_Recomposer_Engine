#include <lynx_public/API/scenes_api.h>
#include <lynx/core/engine_context.h>
#include <lynx/resources/scene_manager.h>

using namespace lynx::engine::api;

void ScenesAPI::Init(const lynx::EngineContext* context){
    p_sceneManager = context->p_sceneManager.get();
}

lynx::resources::scene::AsyncOperationPtr ScenesAPI::LoadSceneAdditiveAsync(const std::string& sceneName) {
    return p_sceneManager->LoadSceneAsync(sceneName, lynx::resources::scene::LoadSceneMode::Additive);
}