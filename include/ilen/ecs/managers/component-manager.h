#pragma once

#include <ilen/ecs/types.h>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <cassert>
#include <vector>
#if ILEN_DEBUG
#include <cxxabi.h>
#endif

namespace ilen::ecs {

    //Interfaz que permite gestionar componentes diferentes de una forma general
    class IComponentArray {
        public:
            virtual ~IComponentArray() = default;
            virtual void removeData(Entity entity) = 0;
            virtual void entityDestroyed(Entity entity) = 0;
            virtual bool hasEntity(Entity entity) = 0;
    };

    //ComponentArray
    template<typename T>
    class ComponentArray : public IComponentArray {
        public:
            void insertData(Entity entity, T&& component) {
                //assert(entityToIndexMap.count(entity) == 0 && "Ya existe el componente");
                // Si ya existe, ignorar en vez de crashear
                if (entityToIndexMap.count(entity) != 0) return;

                size_t newIndex = size;
                entityToIndexMap[entity] = newIndex;
                indexToEntityMap[newIndex] = entity;
                componentArray[newIndex] = std::move(component);
                size++;
            }

            void removeData(Entity entity) override {
                assert(entityToIndexMap.find(entity) != entityToIndexMap.end() && "Tratando de remover un component no existente");

                size_t indexOfRemoved = entityToIndexMap[entity];
                size_t indexOfLast = size - 1;
                componentArray[indexOfRemoved] = std::move(componentArray[indexOfLast]);

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

            bool hasEntity(Entity entity) override {
                return entityToIndexMap.find(entity) != entityToIndexMap.end();
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
            void registerComponent()
            {
                using DecayedType = typename std::decay<T>::type;
                std::type_index typeIndex(typeid(DecayedType));

                if (componentTypes.find(typeIndex) != componentTypes.end()) {
                    return; //Componente ya registrado, ignorar.
                }
                componentTypes.emplace(typeIndex, nextComponentType);
                componentArrays.emplace(typeIndex, std::make_shared<ComponentArray<DecayedType>>());
                nextComponentType++;
            }; //Registra un componente

            template<typename T>
            ComponentType getComponentType(){
                using DecayedType = typename std::decay<T>::type;
                std::type_index typeIndex(typeid(DecayedType));
                if(componentTypes.find(typeIndex) == componentTypes.end()){
                    registerComponent<DecayedType>(); //Registrar el componente porque no existe aún
                }
                return componentTypes[typeIndex];
            }; //Regresa el ID del componente

            template<typename T>
            void addComponent(Entity entity, T&& component){
                using DecayedType = typename std::decay<T>::type;
                registerComponent<DecayedType>();
                auto arrayPtr = std::static_pointer_cast<ComponentArray<DecayedType>>(componentArrays[std::type_index(typeid(DecayedType))]);
                arrayPtr->insertData(entity, std::forward<T>(component));
            }; //Añade un componente a una entidad

            template<typename T>
            void removeComponent(Entity entity){
                using DecayedType = typename std::decay<T>::type;
                auto arrayPointer = std::static_pointer_cast<ComponentArray<DecayedType>>(componentArrays[std::type_index(typeid(DecayedType))]);
                arrayPointer->removeData(entity);
            }; //Elimina un componente a una enitdad

            template<typename T>
            T* getComponent(Entity entity){
                using DecayedType = typename std::decay<T>::type;
                std::type_index typeIndex(typeid(DecayedType));
                if (componentArrays.find(typeIndex) == componentArrays.end()) {
                    // El tipo de componente no está registrado, por lo que ninguna entidad puede tenerlo.
                    return nullptr;
                }
                auto genericArrayPtr = componentArrays.at(typeIndex);
                return std::static_pointer_cast<ComponentArray<DecayedType>>(genericArrayPtr)->getData(entity);
            }; //Regresa la referencia de un componente de una entidad.

            template<typename T>
            bool entityHasComponent(Entity entity){
                using DecayedType = typename std::decay<T>::type;
                std::type_index typeIndex(typeid(DecayedType));
                if (componentArrays.find(typeIndex) == componentArrays.end()) {
                    // El tipo de componente no está registrado, por lo que ninguna entidad puede tenerlo.
                    return false;
                }
                auto genericArrayPtr = componentArrays.at(typeIndex);
                auto result = std::static_pointer_cast<ComponentArray<DecayedType>>(genericArrayPtr)->getData(entity);
                if(result == nullptr)
                    return false; //No encontró ningun puntero valido a ese componente en la entidad.
                else
                    return true; //Se encontró un puntero válido, por tanto, existe el componente en la entidad.
            }

            void entityDestroyed(Entity entity); //Notifica a todos los arrays de componentes que una entidad esta destruida

#ifdef ILEN_DEBUG
            std::vector<std::string> getComponentNamesForEntity(Entity entity) const {
                std::vector<std::string> names;

                for(auto const& [typeIndex, arrayPtr] : componentArrays) {
                    if(arrayPtr -> hasEntity(entity)){
                        names.push_back(demangleTypeName(typeIndex.name()));
                    }
                }

                return names;
            }
#endif

        private:
            std::unordered_map<std::type_index, ComponentType> componentTypes{};
            std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays{};
            ComponentType nextComponentType{};

#ifdef ILEN_DEBUG
            static std::string demangleTypeName(const char* mangledName){
#if defined(__GNUG__) || defined(__clang__)
            int status = 0;
            char* demangled = abi::__cxa_demangle(mangledName, nullptr, nullptr, &status);
            std::string result = (status == 0 && demangled) ? demangled : mangledName;
            free(demangled);
            return result;
#else
            return mangledName;
#endif //__GNUG__, __clang__
            }
#endif //ILEN_DEBUG
    };
}