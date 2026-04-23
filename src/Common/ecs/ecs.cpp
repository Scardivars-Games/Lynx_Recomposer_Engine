#include <ilen/headers/ecs.h>
#ifdef ILEN_DEBUG
#include <sstream>
#endif

using namespace ilen::ecs;

//Coordinator
void ECSCoordinator::init() {
    entityManager = std::make_unique<EntityManager>();
    componentManager = std::make_unique<ComponentManager>();
    systemManager = std::make_unique<SystemManager>();
}

uint32_t noNamedIndex = 0; 

Entity ECSCoordinator::createEntity(){
    std::string newName = "Entity_" + std::to_string(noNamedIndex);
    noNamedIndex++;
    components::EntityMetadataComponent metadata {TagSystem::None, LayerSystem::Default, newName};
    auto entity = entityManager ->createEntity();

    addComponent<components::EntityMetadataComponent>(entity, std::move(metadata));
    return entity;
}

Entity ECSCoordinator::createEntity(components::EntityMetadataComponent&& metadata){
    auto entity = entityManager->createEntity();
    addComponent<components::EntityMetadataComponent>(entity, std::move(metadata));
    return entity;
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
#ifdef ILEN_DEBUG
    liveEntities.insert(id);
#endif
    return id;
}

void EntityManager::destroyEntity(Entity entity) {
    assert(entity < MAX_ENTITIES && "Entidad fuera del rango");
    signatures[entity].reset();
    avalaiblesEntities.push(entity);
    livingEntitiesCount--;
#ifdef ILEN_DEBUG
    liveEntities.erase(entity);
#endif
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

//debug
#ifdef ILEN_DEBUG
using namespace ilen::ecs::debug;
std::string EntityDebugInfo::toString() const {
    std::ostringstream ss;
    ss << "[Entity "<< id << "] name='" << name << "'"
        << " tag=" << tag
        << " layer=" << layer
        << " active=" << (isActive ? "true" : "false")
        << "\n Components(" << components.size() << ")";
    
    for(auto const& comp: components){
        ss << "\n    -"  << comp.typeName;
    }

    return ss.str();
}

std::string ECSDebugSnapshot::toString() const {
    std::ostringstream ss;
    ss << "=== ECS Debug Snapshot === \n"
        << "Total de entidades vivas: " << totalEntities << "\n";

    for(auto const& e: entities){
        ss << e.toString() << "\n";
    }
    return ss.str();
}

ECSDebugSnapshot ECSCoordinator::getDebugSnapshot(){
    ECSDebugSnapshot snapshot;

    auto const& liveEntities = entityManager->getLiveEntities();
    snapshot.totalEntities = static_cast<uint32_t>(liveEntities.size());

    for(Entity entity: liveEntities){
        EntityDebugInfo info;
        info.id = entity;

        auto* meta = getComponent<components::EntityMetadataComponent>(entity);
        if(meta) {
            info.name = meta->getName();
            info.tag = static_cast<uint32_t>(meta->getTags());
            info.layer = static_cast<uint32_t>(meta->getLayers());
            info.isActive = meta->isActive();
        }else{
            info.name = "(NO METADATA)";
        }

        //Componentes
        for(auto const& typeName : componentManager->getComponentNamesForEntity(entity)){
            info.components.push_back({typeName});
        }

        snapshot.entities.push_back(std::move(info));
    }

    return snapshot;
}
#endif