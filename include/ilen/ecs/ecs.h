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
            void addComponent(Entity entity, T&& component){
                using DecayedType = typename std::decay<T>::type;
                componentManager->addComponent<DecayedType>(entity, std::forward<T>(component));
                auto signature = entityManager->getSignature(entity);
                signature.set(componentManager->getComponentType<DecayedType>(), true);
                entityManager->setSignature(entity, signature);
                systemManager->entitySignatureChanged(entity, signature);
            }

            template<typename T>
            T* getComponent(Entity entity){
                return componentManager->getComponent<T>(entity);
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
                componentManager->removeComponent<T>(entity);
                auto signature = entityManager->getSignature(entity);
                signature.set(componentManager->getComponentType<T>(), false);
                entityManager->setSignature(entity, signature);
                systemManager->entitySignatureChanged(entity, signature);
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
        
        private:
            std::unique_ptr<EntityManager> entityManager;
            std::unique_ptr<ComponentManager> componentManager;
            std::unique_ptr<SystemManager> systemManager;
    };
}