#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <ilen/ecs/ecs.h>
#include <ilen/resources/binary_headers.h>
#include <ilen/headers/ilen_logs.h>

namespace ilen::resources::scene {
    class SceneLoader {
        public:
            using DeseralizationCallback = std::function<void(ilen::ecs::Entity, char*&)>;
            SceneLoader(ilen::ecs::ECSCoordinator* coordinator);

            
    };

    class SceneManager {

    };
}