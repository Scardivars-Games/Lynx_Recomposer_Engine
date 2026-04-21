#include <ilen/headers/ecs.h>

using namespace ilen::ecs;

//Coordinator
void ECSCoordinator::init() {
    entityManager = std::make_unique<EntityManager>();
    componentManager = std::make_unique<ComponentManager>();
    systemManager = std::make_unique<SystemManager>();
}

Entity ECSCoordinator::createEntity(){
    return entityManager ->createEntity();
}

void ECSCoordinator::destroyEntity(Entity entity){
    entityManager -> destroyEntity(entity);
    componentManager -> entityDestroyed(entity);
    systemManager -> entityDestroyed(entity);
}

//Entity Manager
EntityManager::EntityManager() {
    //Rellenar toda la cola con todos los posibles IDs
    for(Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
        avalaiblesEntities.push(entity);
    }
}

Entity EntityManager::createEntity(){
    assert(livingEntitiesCount < MAX_ENTITIES && "Demasiadas entidades en el sistema");

    Entity id = avalaiblesEntities.front();
    avalaiblesEntities.pop();
    livingEntitiesCount++;
    return id;
}

void EntityManager::destroyEntity(Entity entity) {
    assert(entity < MAX_ENTITIES && "Entidad fuera del rango");
    signatures[entity].reset();
    avalaiblesEntities.push(entity);
    livingEntitiesCount--;
}

void EntityManager::setSignature(Entity entity, Signature signature){
    assert(entity < MAX_ENTITIES && "Entidad fuera del rango");
    signatures[entity] = signature;
}

Signature EntityManager::getSignature(Entity entity){
    assert(entity < MAX_ENTITIES && "Entidad fuera del rango");
    return signatures[entity];
}

//ComponentManager
void ComponentManager::entityDestroyed(Entity entity){
    for (auto const& pair: componentArrays){
        auto const& componentArray = pair.second;
        componentArray -> entityDestroyed(entity);
    }
}

//System Manager
void SystemManager::entityDestroyed(Entity entity){
    for(auto const& pair : systems){
        auto const& system = pair.second;
        system->entities.erase(entity);
    }
}

void SystemManager::entitySignatureChanged(Entity entity, Signature newSignature) {
    for(auto const& pair: systems){
        auto const& type = pair.first;
        auto const& system = pair.second;
        auto const& systemSignature = signatures[type];

        if((systemSignature & newSignature) == systemSignature){
            system ->entities.insert(entity);
        }else {
            system -> entities.erase(entity);
        }
    }
}

std::vector<std::shared_ptr<ISystem>> SystemManager::getAllSystems() {
    std::vector<std::shared_ptr<ISystem>> list;
    for(auto const& pair : systems){
        list.push_back(pair.second);
    }

    return list;
}

void SystemManager::sortSystems(){
    for(auto& [phase, list] : systemExecutionOrder){
        std::sort(list.begin(), list.end(), [](const SystemConfig& a, const SystemConfig& b){
            return a.priority < b.priority;
        });
    }
}

std::vector<std::shared_ptr<ISystem>> SystemManager::getSystemByPhase(SystemPhase phase){
    std::vector<std::shared_ptr<ISystem>> result;
    for(auto const& config: systemExecutionOrder[phase]){
        result.push_back(config.system);
    }

    return result;
}