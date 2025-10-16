#include <ilen/resources/shaders-manager.h>

namespace ilen::resources::shaders {
    ShaderManager::ShaderManager(EngineContext* context){
        p_jobSystem = context->p_jobSystem.get();
        p_renderer = context->p_gameRenderer.get();
        p_assetsManager = context->p_assetsManager.get();

        //Precargar todos los shaders del motor para que esten disponibles al momento de crear este objeto.
        
    }


}