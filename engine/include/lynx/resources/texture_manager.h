#pragma once

//Encargado de administar el uso de las texturas durante todo el ciclo de vida de la aplicación.
#include <lynx/job_system/job_system.h>
#include <lynx/render/renderer.h>
#include <lynx/core/lifetime.h>
#include <lynx/graphics/texture.h>
#include <unordered_map>
#include <future>

namespace lynx::resources {
    class AssetsManager;
}

namespace lynx {
    struct EngineContext;
}

namespace lynx::render
{
    class IRenderer;
}

namespace lynx::resources::texture {
    //Almacena los datos necesarios para las llamadas a la GPU.
    using namespace lynx::job_system;
    using namespace lynx::render;
    //using namespace lynx::core;
    using namespace lynx::graphics;

    using TextureID = uint32_t; //Solamente para legebilidad. Sigue siendo un uint32_t como el resto de IDs que usa el motor.
    using RuntimeTextureID = uint16_t;

    class TextureManager {
        public:
            explicit TextureManager(EngineContext* engineContext);
            ~TextureManager() = default;

            TextureManager(const TextureManager&) = delete;
            TextureManager& operator=(const TextureManager&) = delete;

            void UpdateProcess();
            float LoadingStatus();
            std::weak_ptr<TextureGPULink> getTextureHandler(TextureID id);
            uint16_t getTextureRuntimeID(TextureID id) const;
        private:
            std::unordered_map<TextureID, std::shared_ptr<TextureGPULink>> m_texBufferData;
            std::unordered_map<TextureID, std::future<TextureGPULink>> m_texLoadingData;
            std::unordered_map<TextureID, RuntimeTextureID> m_texRuntimeID;

            uint32_t lastTimeAmountOfElementLoading = 0;

            JobSystem* jobSystem = nullptr;
            AssetsManager* assetsManager = nullptr;
            IRenderer* renderer = nullptr;
            mutable std::mutex m_mutex;

            uint16_t m_nextRuntimeID = 1;

            const char* DEBUG_PREFIX = "TEXTURE_MANAGER";
    };
}