#include <lynx/game_logic/script_registry.h>

#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace lynx::game {
    namespace {
        using RegistryMap = std::unordered_map<std::string, ScriptBindFunc>;

        RegistryMap& GetRegistryStorage() {
            static RegistryMap registry;
            return registry;
        }

        std::mutex& GetRegistryMutex() {
            static std::mutex mutex;
            return mutex;
        }
    }

    void ScriptRegistry::RegisterFactory(std::string name, ScriptBindFunc factory) {
        std::lock_guard lock(GetRegistryMutex());

        GetRegistryStorage().insert_or_assign(std::move(name), std::move(factory));
    }

    bool ScriptRegistry::TryBind(const std::string &name, lynx::ecs::components::ScriptingBridgeComponent &comp) {
        ScriptBindFunc factory;

        {
            std::lock_guard lock(GetRegistryMutex());

            const auto iterator = GetRegistryStorage().find(name);

            if(iterator == GetRegistryStorage().end()) {
                return false;
            }

            factory = iterator->second;
        }

        factory(comp);
        return true;
    }
}