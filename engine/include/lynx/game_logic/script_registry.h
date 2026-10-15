#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <lynx/game_logic/ecs_bridge.h>

namespace lynx::game {
    using ScriptBindFunc = std::function<void(lynx::ecs::components::ScriptingBridgeComponent&)>;

    class ScriptRegistry final {
        public:
            static void RegisterFactory(std::string name, ScriptBindFunc factory);

            template<typename T>
            static void Register(const std::string& name) {
                RegisterFactory(name, [](lynx::ecs::components::ScriptingBridgeComponent& component){
                    component.bind<T>();
                });
            }

            static bool TryBind(const std::string& name, lynx::ecs::components::ScriptingBridgeComponent& comp);

    };

    template<typename T>
    struct AutoRegisterScript {
        AutoRegisterScript(const char* name) {
            ScriptRegistry::Register<T>(name);
        }
    };
}

#define LYNX_CONCAT_IMPL(x, y) x##y
#define LYNX_CONCAT(x, y) LYNX_CONCAT_IMPL(x, y)

#define LYNX_REGISTER_SCRIPT(Type) \
    inline static lynx::game::AutoRegisterScript<Type> LYNX_CONCAT(auto_reg_script_, __LINE__)(#Type)
    