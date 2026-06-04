#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <ilen/game_logic/ecs_bridge.h>

namespace ilen::game {
    using ScriptBindFunc = std::function<void(ilen::ecs::components::ScriptingBridgeComponent&)>;

    class ScriptRegistry {
        private:
            static inline std::unordered_map<std::string, ScriptBindFunc> s_Registry;

        public:
            template<typename T>
            static void Register(const std::string& name) {
                s_Registry[name] = [](ilen::ecs::components::ScriptingBridgeComponent& comp) {
                    comp.bind<T>();
                };
            }

            static bool TryBind(const std::string& name, ilen::ecs::components::ScriptingBridgeComponent& comp){
                if(s_Registry.contains(name)){
                    s_Registry[name](comp);
                    return true;
                }
                return false;
            }
    };

    template<typename T>
    struct AutoRegisterScript {
        AutoRegisterScript(const char* name) {
            ScriptRegistry::Register<T>(name);
        }
    };
}

#define ILEN_CONCAT_IMPL(x, y) x##y
#define ILEN_CONCAT(x, y) ILEN_CONCAT_IMPL(x, y)

#define ILEN_REGISTER_SCRIPT(Type) \
    inline static ilen::game::AutoRegisterScript<Type> ILEN_CONCAT(auto_reg_script_, __LINE__)(#Type)