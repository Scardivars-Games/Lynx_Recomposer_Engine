#pragma once

#include <ilen/ecs/types.h>
#include <memory>
#include <typeindex>
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
                assert(entityToIndexMap.count(entity) == 0 && "Ya existe el componente");

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

            T* getData(Entity entity) {
                if (entityToIndexMap.find(entity) == entityToIndexMap.end()) {
                    return nullptr;
                }

                return &componentArray.at(entityToIndexMap.at(entity));
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
            size_t size = 0;
    };

    class ComponentManager {
        public:
            template<typename T>
            void registerComponent() {
                //const char* typeName = typeid(T).name();
                std::type_index typeIndex(typeid(T));

                if(componentTypes.find(typeIndex) != componentTypes.end()){
                    return; //Componente ya registrado, ignorar.
                }
                componentTypes.emplace(typeIndex, nextComponentType);
                componentArrays.emplace(typeIndex, std::make_shared<ComponentArray<T>>());
                nextComponentType++;
            }; //Registra un componente

            template<typename T>
            ComponentType getComponentType(){
                std::type_index typeIndex(typeid(T));
                if(componentTypes.find(typeIndex) == componentTypes.end()){
                    registerComponent<T>(); //Registrar el componente porque no existe aún
                }
                return componentTypes[typeIndex];
            }; //Regresa el ID del componente

            template<typename T>
            void addComponent(Entity entity, T component){
                registerComponent<T>();
                auto arrayPtr = std::static_pointer_cast<ComponentArray<T>>(componentArrays[std::type_index(typeid(T))]);
                arrayPtr->insertData(entity, component);
            }; //Añade un componente a una entidad

            template<typename T>
            void removeComponent(Entity entity){
                //std::static_pointer_cast<ComponentArray<T>>(componentArrays[std::type_index(typeid(T))]->removeData(entity));
                auto arrayPointer = std::static_pointer_cast<ComponentArray<T>>(componentArrays[std::type_index(typeid(T))]);
                arrayPointer->removeData(entity);
            }; //Elimina un componente a una enitdad

            template<typename T>
            T* getComponent(Entity entity){
                std::type_index typeIndex(typeid(T));
                if (componentArrays.find(typeIndex) == componentArrays.end()) {
                    // El tipo de componente no está registrado, por lo que ninguna entidad puede tenerlo.
                    return nullptr;
                }
                auto genericArrayPtr = componentArrays.at(typeIndex);
                return std::static_pointer_cast<ComponentArray<T>>(genericArrayPtr)->getData(entity);
            }; //Regresa la referencia de un componente de una entidad.

            void entityDestroyed(Entity entity); //Notifica a todos los arrays de componentes que una entidad esta destruida

        private:
            std::unordered_map<std::type_index, ComponentType> componentTypes{};
            std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays{};
            ComponentType nextComponentType{};
    };
}