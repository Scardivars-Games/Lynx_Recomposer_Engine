#include <ilen-public/API/entity-api.h>

using namespace ilen::engine::api;

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

void EntityAPI::Init(const ilen::EngineContext* context){
    p_coordinator = context->p_coordinator.get();
}

void EntityAPI::GetMetadataPointer(){
    if(metadataPtr == nullptr)
        metadataPtr = p_coordinator->getComponentArray<components::EntityMetadataComponent>();
}

void EntityAPI::ResetMetadataPointer(){
    metadataPtr = nullptr;
}