#pragma once

#include <ilen/ecs/types.h>
#include <ilen/headers/ecs_managers.h>
#include <ilen/core/engine_context.h>
#include <memory>
#include <vector>

#ifdef ILEN_DEBUG
namespace ilen::ecs::debug{
    struct ComponentDebugInfo {
        std::string typeName;
    };

    struct EntityDebugInfo{
        Entity id;
        std::string name;
        uint32_t tag;
        uint32_t layer;
        uint64_t uuid;
        bool isActive;
        std::vector<ComponentDebugInfo> components;

        std::string toString() const;
    };

    struct ECSDebugSnapshot {
        std::vector<EntityDebugInfo> entities;
        uint32_t totalEntities;
        std::string toString() const;
    };
}
#endif

namespace ilen::ecs {
    class EntityManager;
    class ComponentManager;
    class SystemManager;



    class ECSCoordinator {
        public:
            void init(const ilen::EngineContext* engineContext);

            //Entidaded
            Entity createEntity();
            Entity createEntity(components::EntityMetadataComponent&& metadata);
            Entity getEntityByName(std::string name);
            void destroyEntity(Entity entity);

            //Componentes
            template<typename T>
            void registerComponent() {
                componentManager->registerComponent<T>();
            }

            template<typename T>
            void setComponent(Entity entity, T&& component){
                if(ilen::core::threading::t_IsMainThread){
                    using DecayedType = typename std::decay<T>::type;
                    bool wasNew = !hasComponent<DecayedType>(entity);

                    componentManager->setComponent<DecayedType>(entity, std::forward<T>(component));

                    if(wasNew){
                        auto signature = entityManager->getSignature(entity);
                        signature.set(componentManager->getComponentType<DecayedType>(), true);
                        entityManager->setSignature(entity, signature);
                        systemManager->entitySignatureChanged(entity, signature);
                    }
                } else {
                    auto& localBuffer = p_commandDispatcher->getBuffer(ilen::core::threading::t_ThreadIndex);
                    localBuffer.push([this, entity, comp = std::forward<T>(component)]() mutable {
                        this->setComponent<T>(entity, std::move(comp));
                    });
                }
            }

            template<typename T>
            T* getComponent(Entity entity){
                return componentManager->getComponent<T>(entity);
            }

            template<typename T>
            ComponentArray<typename std::decay<T>::type>* getComponentArray(){
                return componentManager->getComponentArray<T>();
            }

            template<typename T>
            ComponentType getComponentType() {
                return componentManager->getComponentType<T>();
            }

            template<typename T>
            bool hasComponent(Entity entity){
                return componentManager->entityHasComponent<T>(entity);
            }

            //Sistema
            template<typename T>
            std::shared_ptr<T> registerSystem(SystemPhase phase, int prioriy) {
                return systemManager -> registerSystem<T>(phase, prioriy);
            }

            template<typename T>
            void setSystemSignature(Signature signature){
                systemManager->setSignature<T>(signature);
            } 

            template<typename T>
            void removeComponent(Entity entity){
                if(ilen::core::threading::t_IsMainThread){
                    componentManager->removeComponent<T>(entity);
                    auto signature = entityManager->getSignature(entity);
                    signature.set(componentManager->getComponentType<T>(), false);
                    entityManager->setSignature(entity, signature);
                    systemManager->entitySignatureChanged(entity, signature);
                } else {
                    auto& localBuffer = p_commandDispatcher->getBuffer(ilen::core::threading::t_ThreadIndex);

                    localBuffer.push([this, entity]() mutable {
                        this->removeComponent<T>(entity);
                    });
                }
            }

            template<typename T>
            std::shared_ptr<T> getSystem() {
                return systemManager->getSystem<T>();
            }

            std::vector<std::shared_ptr<ISystem>> getAllSystems(){
                return systemManager->getAllSystems();
            }

            void sortSystems(){
                systemManager->sortSystems();
            }

            std::vector<std::shared_ptr<ISystem>> getSystemsByPhase(SystemPhase phase){
                return systemManager->getSystemByPhase(phase);
            }
        
#ifdef ILEN_DEBUG
            ilen::ecs::debug::ECSDebugSnapshot getDebugSnapshot();
#endif

        private:
            std::unique_ptr<EntityManager> entityManager;
            std::unique_ptr<ComponentManager> componentManager;
            std::unique_ptr<SystemManager> systemManager;

            //Para localizar rápidamente entidad por Nombre
            std::unordered_map<std::string, Entity> m_nameToEntityMap;
            
            ilen::job_system::CommandBufferDispatcher* p_commandDispatcher = nullptr;
    };
}