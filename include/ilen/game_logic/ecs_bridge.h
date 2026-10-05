//Une la lógica de un GameObject con su respectiva Entidad en el ECS.
#pragma once

#if !ILEN_ONLY_ECS

#include <memory>
#include <functional>
#include <ilen/game_logic/game_object.h>

namespace ilen::ecs::components {

    ILEN_COMPONENT
    struct ScriptingBridgeComponent {
        ILEN_PROPERTY std::string scriptName = "";

        std::shared_ptr<ilen::game::GameObject> instance = nullptr;

        std::function<std::shared_ptr<ilen::game::GameObject>()> instantiate;
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
namespace ilen::ecs::components {
    struct ScriptingBridgeComponent {
        std::string scriptName = "";
    };
}
#endif //!ILEN_ONLY_ECS