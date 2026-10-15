//Une la lógica de un GameObject con su respectiva Entidad en el ECS.
#pragma once

#if !LYNX_ONLY_ECS

#include <memory>
#include <functional>
#include <lynx/game_logic/game_object.h>

namespace lynx::ecs::components {

    LYNX_COMPONENT
    struct ScriptingBridgeComponent {
        LYNX_PROPERTY std::string scriptName = "";

        std::shared_ptr<lynx::game::GameObject> instance = nullptr;

        std::function<std::shared_ptr<lynx::game::GameObject>()> instantiate;
        std::function<void()> destroyScript;

        template<typename T>
        void bind(){
            instantiate = []() { return std::make_shared<T>(); };
            //destroyScript = [&]() { instance.reset(); };
        }

        void destroy() {
            instance.reset();
        }
    };
}

#else

#include <string>
//Compatibilidad
namespace lynx::ecs::components {
    struct ScriptingBridgeComponent {
        std::string scriptName = "";
    };
}
#endif //!LYNX_ONLY_ECS