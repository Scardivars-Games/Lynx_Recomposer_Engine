#include <lynx_public/API/entity_api.h>
#include <lynx/resources/scene_manager.h>
#include <string>
#include "lynx/ecs/types.h"

using namespace lynx::engine::api;

void EntityAPI::SetActive(Entity entity, bool newState){
    GetMetadataPointer();
    auto* meta = metadataPtr->getData(entity);
    meta->setActive(newState);
}

bool EntityAPI::isActive(Entity entity){
    GetMetadataPointer();
    return metadataPtr->getData(entity)->isActive();
}

Entity EntityAPI::GetEntityByName(std::string entityName){
    return p_coordinator->getEntityByName(entityName);
}

bool EntityAPI::entityHasTag(Entity entity, TagSystem tag) {
    GetMetadataPointer();
    return metadataPtr->getData(entity)->hasTag(tag);
}

std::string EntityAPI::GetEntityName(Entity entity) {
    GetMetadataPointer();
    return metadataPtr->getData(entity)->getName();
}

void EntityAPI::Init(const lynx::EngineContext* context){
    p_coordinator = context->p_coordinator.get();
    p_sceneManager = context->p_sceneManager.get();
}

void EntityAPI::GetMetadataPointer(){
    if(metadataPtr == nullptr)
        metadataPtr = p_coordinator->getComponentArray<components::EntityMetadataComponent>();
}

void EntityAPI::ResetMetadataPointer(){
    metadataPtr = nullptr;
}

void EntityAPI::LoadPrefab(std::string prefabName) {
    p_sceneManager->LoadPrefab(prefabName);
}

void EntityAPI::LoadPrefab(std::string prefabName, vec3 prefabPosition) {
    p_sceneManager->LoadPrefab(prefabName, prefabPosition);
}