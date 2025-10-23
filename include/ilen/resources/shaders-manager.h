//Administra la carga de los shaders según haga falta (y si el OS lo requiere) así como cada Render Pipeline creado a partir de estos shaders.
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <ilen/core/filemanager.h>

#include <ilen/render/gpu-interfaces/shaders.h>
#include <ilen/job-system/job-system.h>
#include <ilen/core/lifetime.h>

namespace ilen
{
    struct  EngineContext;
}

namespace ilen::resources
{
    class AssetsManager;
    using namespace ilen::enumerations;

    template<ILEN_ASSET_TYPE T>
    struct AssetElement;

    using ShaderAsset = AssetElement<ILEN_ASSET_TYPE_SHADER>; // El 4 corresponde a ILEN_ASSET_TYPE_SHADER
}

namespace ilen::render
{
    class IShader;
    class IRenderer;
}


namespace ilen::resources::shaders {
    using namespace ilen::render;
    using namespace ilen::job_system;

    using ShaderID = uint32_t; // Solo para legibilidad.

    class ShaderManager {
        public:
            explicit ShaderManager(EngineContext* context);

            std::weak_ptr<IShader> getShaderData(ShaderID id);
        private:
            void LoadShaderData(const ShaderAsset& asset); //Esta operación es un fallback en caso de que algún shader no se cargue al iniciar el método. NINGUNA OTRA CLASE DEL MOTOR DEBE USARLA


            std::unordered_map<ShaderID, std::shared_ptr<IShader>> m_shaderData;

            JobSystem* p_jobSystem;
            IRenderer* p_renderer;
            AssetsManager* p_assetsManager;

            const char* DEBUG_PREFIX = "SHADER MANAGER";
    };
}
