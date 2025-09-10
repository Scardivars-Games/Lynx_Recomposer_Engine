#pragma once

#include <ilen/ecs/types.h>
#include <ilen/headers/ecs-managers.h>
#include <memory>
#include <vector>

namespace ilen::ecs {
    class EntityManager;
    class ComponentManager;
    class SystemManager;

    class ECSCoordinator {
        public:
            void init();

            //Entidaded
            Entity createEntity();
            void destroyEntity(Entity entity);

            //Componentes
            template<typename T>
            void registerComponent() {
                componentManager->registerComponent<T>();
            }

            template<typename T>
            void addComponent(Entity entity, T component){
                componentManager->addComponent<T>(entity, component);
                auto signature = entityManager->getSignature(entity);
                signature.set(componentManager->getComponentType<T>(), true);
                entityManager->setSignature(entity, signature);
                systemManager->entitySignatureChanged(entity, signature);
            }

            template<typename T>
            T& getComponent(Entity entity){
                return componentManager->getComponent<T>(entity);
            }

            template<typename T>
            ComponentType getComponentType() {
                return componentManager->getComponentType<T>();
            }

            //Sistema
            template<typename T>
            std::shared_ptr<T> registerSystem() {
                return systemManager -> registerSystem<T>();
            }

            template<typename T>
            void setSystemSignature(Signature signature){
                systemManager->setSignature<T>(signature);
            } 
        
        private:
            std::unique_ptr<EntityManager> entityManager;
            std::unique_ptr<ComponentManager> componentManager;
            std::unique_ptr<SystemManager> systemManager;
    };
}