#pragma once

#include <ilen/ecs/types.h>
#include <memory>
#include <unordered_map>
#include <cassert>

namespace ilen::ecs {

    //Interfaz que permite gestionar componentes diferentes de una forma general
    class IComponentArray {
        public:
            ~IComponentArray() = default;
            //virtual void insertData() = 0;
            virtual void removeData(Entity entity) = 0;
            //virtual void* getData(Entity entity) = 0;
            virtual void entityDestroyed(Entity entity) = 0;
    };

    //ComponentArray
    template<typename T>
    class ComponentArray : public IComponentArray {
        public:
            void insertData(Entity entity, T component) {
                assert(entityToIndexMap.find(entity) == entityToIndexMap.end() && "Tratando de añadir el mismo componente a la misma entidad");

                size_t newIndex = size;
                entityToIndexMap[entity] = newIndex;
                indexToEntityMap[newIndex] = entity;
                componentArray[newIndex] = component;
                size++;
            }

            void removeData(Entity entity) override {
                assert(entityToIndexMap.find(entity) != entityToIndexMap.end() && "Tratando de remover un component no existente");

                size_t indexOfRemoved = entityToIndexMap[entity];
                size_t indexOfLast = size - 1;
                componentArray[indexOfRemoved] = componentArray[indexOfLast];

                Entity entityOfLast = indexToEntityMap[indexOfLast];
                entityToIndexMap[entityOfLast] = indexOfRemoved;
                indexToEntityMap[indexOfRemoved] = entityOfLast;

                entityToIndexMap.erase(entity);
                indexToEntityMap.erase(indexOfLast);
                size--;
            }

            T& getData(Entity entity) {
                assert(entityToIndexMap.find(entity) != entityToIndexMap.end() && "Tratando de obtener un componente no existente");
                return componentArray[entityToIndexMap[entity]];
            }

            void entityDestroyed(Entity entity) override {
                if(entityToIndexMap.find(entity) != entityToIndexMap.end()){
                    removeData(entity);
                }
            }

        private:
            std::array<T, MAX_ENTITIES> componentArray;
            std::unordered_map<Entity, size_t> entityToIndexMap;
            std::unordered_map<size_t, Entity> indexToEntityMap;
            size_t size;
    };

    class ComponentManager {
        public:
            template<typename T>
            void registerComponent() {
                const char* typeName = typeid(T).name();
                assert(componentTypes.find(typeName) == componentTypes.end() && "Componente ya registrado");

                componentTypes.insert({typeName, nextComponentType});
                componentArrays.insert({typeName, std::make_shared<ComponentArray<T>>()});
                nextComponentType++;
            }; //Registra un componente

            template<typename T>
            ComponentType getComponentType(){
                const char* typeName = typeid(T).name();
                assert(componentTypes.find(typeName) != componentTypes.end() && "Componente no se encuentra registrado y se esta usando");
                return componentTypes[typeName];
            }; //Regresa el ID del componente

            template<typename T>
            void addComponent(Entity entity, T component){
                auto generiArrayPtr = componentArrays[typeid(T).name()];
                auto specificArrayPtr = std::static_pointer_cast<ComponentArray<T>>(generiArrayPtr);
                specificArrayPtr->insertData(entity, component);
                //std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeid(T).name()]->insertData(entity, component));
            }; //Añade un componente a una entidad

            template<typename T>
            void removeComponent(Entity entity){
                std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeid(T).name()]->removeData(entity));
            }; //Elimina un componente a una enitdad

            template<typename T>
            T& getComponent(Entity entity){
                return std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeid(T).name()])->getData(entity);
            }; //Regresa la referencia de un componente de una entidad.

            void entityDestroyed(Entity entity); //Notifica a todos los arrays de componentes que una entidad esta destruida

        private:
            std::unordered_map<const char*, ComponentType> componentTypes{};
            std::unordered_map<const char*, std::shared_ptr<IComponentArray>> componentArrays{};
            ComponentType nextComponentType{};
    };
}