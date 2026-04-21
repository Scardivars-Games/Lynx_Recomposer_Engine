//Une la lógica de un GameObject con su respectiva Entidad en el ECS.
#pragma once

#if !ILEN_ONLY_ECS

#include <memory>
#include <functional>
#include <ilen/game-logic/game-object.h>

namespace ilen::ecs::components {
    struct ScriptingBridgeComponent {
        std::shared_ptr<ilen::game::GameObject> instance = nullptr;

        std::function<std::shared_ptr<ilen::game::GameObject>()> instantiate;
        std::function<void()> destroyScript;

        template<typename T>
        void bind(){
            instantiate = []() { return std::make_shared<T>(); };
            destroyScript = [&]() { instance.reset(); };
        }
    };
}

#endif //!ILEN_ONLY_ECS