#pragma once

#include <string>
#include <lynx/resources/scene_manager.h>
#include <lynx/ecs/types.h>

namespace lynx::engine::api {
    class ScenesAPI {
        public:
            //Carga, usando single un único nivel congelando el hilo.
            static void LoadScene(const std::string& sceneName);

            static resources::scene::AsyncOperationPtr LoadSceneAsync(const std::string& sceneName);
            static resources::scene::AsyncOperationPtr LoadSceneAdditiveAsync(const std::string& sceneName);

            static void UnloadScene(const std::string& sceneName);

            static void DontDestroyOnLoad(lynx::ecs::Entity entity);

            static void Init(const lynx::EngineContext* context);
        private:
            static inline lynx::resources::scene::SceneManager* p_sceneManager;
    };
}