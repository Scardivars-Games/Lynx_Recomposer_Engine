//Administra la carga de los shaders según haga falta (y si el OS lo requiere) así como cada Render Pipeline creado a partir de estos shaders.
#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include <ilen/render/gpu-interfaces/shaders.h>
#include <ilen/job-system/job-system.h>

namespace ilen::resources::shaders {
    using namespace ilen::render;
    using namespace ilen::job_system;

    using ShaderID = uint32_t; // Solo para legibilidad.

    class ShaderManager {
        public:
            ShaderManager(EngineContext* context);

            std::weak_ptr<IShader> getShaderData(ShaderID id);
            void LoadShaderData(ShaderID id); //Esta operación es un fallback en caso de que algún shader no se cargue al iniciar el método.
        private:
            std::unordered_map<ShaderID, std::unique_ptr<IShader>> m_shaderData;

            JobSystem* p_jobSystem;
            Renderer* p_renderer;
            AssetsManager* p_assetsManager;
    };
}
