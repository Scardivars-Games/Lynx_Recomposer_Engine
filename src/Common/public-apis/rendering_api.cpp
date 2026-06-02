#include <ilen_public/API/rendering_api.h>

using namespace ilen::ecs;
using namespace ilen::engine::api;
using namespace ilen::maths;
using namespace ilen::ecs::components;

void RenderingAPI::Init(const ilen::EngineContext* context) {
    p_coordinator = context->p_coordinator.get();
    GetComponentPointers();
}

void RenderingAPI::GetComponentPointers(){
    //Tecnicamente, una vez creado los arrays, estos nunca deberían de cambiar de dirección en memoria.
    components.renderComponentArray = p_coordinator->getComponentArray<RenderComponent>();
    components.staticMaterialArray = p_coordinator->getComponentArray<StaticMaterialRenderingData>();
    components.staticRenderingArray = p_coordinator->getComponentArray<StaticRenderingData>();
    components.staticTextureArray = p_coordinator->getComponentArray<StaticTextureRenderingData>();
}

void RenderingAPI::ResetComponentsPointers() {
    components.renderComponentArray = nullptr;
    components.staticMaterialArray = nullptr;
    components.staticRenderingArray = nullptr;
    components.staticTextureArray = nullptr;
}

void RenderingAPI::FlipSpriteInX(Entity e, bool value){
    if(components.staticTextureArray == nullptr){
        GetComponentPointers();
    }

    auto* component = components.staticTextureArray->getData(e);
    if(component == nullptr){
        ILEN_ERROR_P(DEBUG_PREFIX, "El component con ID %e no contiene un componente de textura. No se puede modificar el Flip.", e);
        return;
    }

    component->flipX = value;
}

void RenderingAPI::FlipSpriteInY(Entity e, bool value){
    if(components.staticTextureArray == nullptr){
        GetComponentPointers();
    }

    auto* component = components.staticTextureArray->getData(e);
    if(component == nullptr){
        ILEN_ERROR_P(DEBUG_PREFIX, "El component con ID %e no contiene un componente de textura. No se puede modificar el Flip.", e);
        return;
    }

    component->flipY = value;
}

void RenderingAPI::ChangeSpriteIndex(Entity e, int index){
    if(components.staticTextureArray == nullptr){
        GetComponentPointers();
    }

    auto* component = components.staticTextureArray->getData(e);
    if(component == nullptr){
        ILEN_ERROR_P(DEBUG_PREFIX, "El component con ID %e no contiene un componente de textura. No se puede cambiar el indice.", e);
        return;
    }

    component->currentSpriteID = index;
}