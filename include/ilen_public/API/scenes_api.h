#pragma once

#include <string>
#include <ilen/resources/scene_manager.h>
#include <ilen/ecs/types.h>

namespace ilen::engine::api {
    class ScenesAPI {
        public:
            //Carga, usando single un único nivel congelando el hilo.
            static void LoadScene(const std::string& sceneName);

            static resources::scene::AsyncOperationPtr LoadSceneAsync(const std::string& sceneName);
            static resources::scene::AsyncOperationPtr LoadSceneAdditiveAsync(const std::string& sceneName);

            static void UnloadScene(const std::string& sceneName);

            static void DontDestroyOnLoad(ilen::ecs::Entity entity);

            static void Init(const ilen::EngineContext* context);
        private:
            static inline ilen::resources::scene::SceneManager* p_sceneManager;
    };
}