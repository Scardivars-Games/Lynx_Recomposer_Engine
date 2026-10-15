#pragma once

#include <cstdint>

#include <lynx/core/application.h>
#include <lynx/core/runtime_configuration.h>

namespace lynx::resources::scene {
    class SceneManager;
}

namespace lynx::core {
    using ReflectionRegistrationCallback = void(*)(lynx::resources::scene::SceneManager&);

    struct ProjectBootsrap {
        lynx::application::ApplicationMetadata metadata{};
        lynx::core::RuntimeConfiguration runtimeConfiguration{};

        std::uint32_t initialScene = 0;

        ReflectionRegistrationCallback registerReflection = nullptr;
    };
}