#include <lynx/headers/ecs.h>
#ifdef LYNX_DEBUG
#include <sstream>
#endif

using namespace lynx::ecs;

//Coordinator
void ECSCoordinator::init(const lynx::EngineContext* engineContext) {
    const Entity maxEntities = engineContext->runtimeConfiguration->ecs.initialEntityCapacity;

    if(maxEntities == 0)
        LYNX_EXECUTE_ERROR_P("ECS_CORE", "No se puede ejecutar un juego con un tamaño máximo de 0 entidades. Revise las opciones del proyecto, ajuste el valor y recompile el juego. En caso contrario, contacte al desarrollador por soporte.");
    
    entityManager = std::make_unique<EntityManager>(maxEntities);
    componentManager = std::make_unique<ComponentManager>(maxEntities);
    systemManager = std::make_unique<SystemManager>();

    p_commandDispatcher = engineContext->p_commandDispatcher.get();
}

uint32_t noNamedIndex = 0; 

Entity ECSCoordinator::createEntity(){
    std::string newName = "Entity_" + std::to_string(noNamedIndex);
    noNamedIndex++;
    components::EntityMetadataComponent metadata {TagSystem::None, LayerSystem::Default, newName};
    auto entity = entityManager -> createEntity();

    setComponent<components::EntityMetadataComponent>(entity, std::move(metadata));

    m_nameToEntityMap[newName] = entity;
    return entity;
}

Entity ECSCoordinator::createEntity(components::EntityMetadataComponent&& metadata){
    auto entity = entityManager->createEntity();
    auto entityName = metadata.getName();
    setComponent<components::EntityMetadataComponent>(entity, std::move(metadata));

    m_nameToEntityMap[entityName] = entity;
    return entity;
}

Entity ECSCoordinator::getEntityByName(std::string name){
    if(m_nameToEntityMap.count(name)){
        return m_nameToEntityMap[name];
    }

    LYNX_ERROR_P("ENTITY_API", "No existe una entidad con el nombre %s en la escena actual del motor", name.c_str());
    return NULL_ENTITY;
}

void ECSCoordinator::destroyEntity(Entity entity){
    entityManager -> destroyEntity(entity);
    componentManager -> entityDestroyed(entity);
    systemManager -> entityDestroyed(entity);
}

//Entity Manager
EntityManager::EntityManager(Entity maxEntities) : m_maxEntities(maxEntities), m_signatures(maxEntities) {
    if(m_maxEntities == 0) {
        LYNX_EXECUTE_ERROR_P("ECS_CORE", "No se puede ejecutar un juego con un tamaño máximo de 0 entidades. Revise las opciones del proyecto, ajuste el valor y recompile el juego. En caso contrario, contacte al desarrollador por soporte.");
    }

    for(Entity entity = 0;entity < m_maxEntities; entity++) {
        m_avalaiblesEntities.push(entity);
    }
}

void EntityManager::validateEntity(Entity entity) const {
    if(entity >= m_maxEntities) {
        LYNX_EXECUTE_ERROR_P("ECS_CORE","Entidad con ID %u mayor que el ID máximo: %u", entity, m_maxEntities);
    }
}

Entity EntityManager::createEntity(){
    if (m_livingEntitiesCount >= m_maxEntities) LYNX_EXECUTE_ERROR_P("ECS_CORE", "Demasiadas entidades en el sistema");

    const Entity id = m_avalaiblesEntities.front();
    m_avalaiblesEntities.pop();
    m_livingEntitiesCount++;
#ifdef LYNX_DEBUG
    m_liveEntities.insert(id);
#endif
    return id;
}

void EntityManager::destroyEntity(Entity entity) {
    validateEntity(entity);

    m_signatures[entity].reset();
    m_avalaiblesEntities.push(entity);
    m_livingEntitiesCount--;
#ifdef LYNX_DEBUG
    m_liveEntities.erase(entity);
#endif
}

void EntityManager::setSignature(Entity entity, Signature signature){
    validateEntity(entity);

    m_signatures[entity] = signature;
}

Signature EntityManager::getSignature(Entity entity){
    validateEntity(entity);
    return m_signatures[entity];
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

void ECSCoordinator::UpdateEntityName(lynx::ecs::Entity entity, const std::string& oldName, const std::string& newName){
    if(oldName == newName) return;

    auto it = m_nameToEntityMap.find(oldName);
    //Borrar el nombre viejo tras confirmar que apunte a la misma entidad
    if(it != m_nameToEntityMap.end() && it->second == entity) {
        m_nameToEntityMap.erase(it);
    }

    //Registrar el nuevo
    if(!newName.empty()) {
        m_nameToEntityMap[newName] = entity;
    }
}


//debug
#ifdef LYNX_DEBUG
using namespace lynx::ecs::debug;
std::string EntityDebugInfo::toString() const {
    std::ostringstream ss;
    ss << "[Entity "<< id << "] name='" << name << "'"
        << " uuid=" << uuid
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
            info.uuid = static_cast<uint64_t>(meta->uuid);
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