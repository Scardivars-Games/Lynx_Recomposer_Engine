//Administra la carga de los shaders según haga falta (y si el OS lo requiere) así como cada Render Pipeline creado a partir de estos shaders.
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <lynx/core/filemanager.h>

#include <lynx/render/gpu_interfaces/shaders.h>
#include <lynx/job_system/job_system.h>
#include <lynx/core/lifetime.h>

namespace lynx
{
    struct  EngineContext;
}

namespace lynx::resources
{
    class AssetsManager;
    using namespace lynx::enumerations;

    template<LYNX_ASSET_TYPE T>
    struct AssetElement;

    using ShaderAsset = AssetElement<LYNX_ASSET_TYPE_SHADER>; // El 4 corresponde a LYNX_ASSET_TYPE_SHADER
}

namespace lynx::render
{
    class IShader;
    class IRenderer;
}


namespace lynx::resources::shaders {
    using namespace lynx::render;
    using namespace lynx::job_system;

    using ShaderID = uint32_t; // Solo para legibilidad.

    class ShaderManager {
        public:
            explicit ShaderManager(EngineContext* context);

            std::weak_ptr<IShader> getShaderData(ShaderID id);
        private:
            void LoadShaderData(const ShaderAsset& asset); //Esta operación es un fallback en caso de que algún shader no se cargue al iniciar el método. NINGUNA OTRA CLASE DEL MOTOR DEBE USARLA


            std::unordered_map<ShaderID, std::shared_ptr<IShader>> m_shaderData;
            std::mutex m_mutex;

            JobSystem* p_jobSystem;
            IRenderer* p_renderer;
            AssetsManager* p_assetsManager;
            core::RuntimeConfiguration* p_runtimeConfig;

            const char* DEBUG_PREFIX = "SHADER MANAGER";
    };
}
