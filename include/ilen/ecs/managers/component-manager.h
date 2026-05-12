#pragma once

#include <ilen/ecs/types.h>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <cassert>
#include <vector>
#include <ilen/headers/ilen-logs.h>
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
//COMPROBACIONES PARA VER SI VALE LA PENA EL ALIGNAS
#if ILEN_DEBUG
            static void PrintDiagnostics(){
                int status = 0;
                const char* mangledName = typeid(T).name();
#if defined(__GNUG__) || defined(__clang__)
                char* demangled = abi::__cxa_demangle(mangledName, nullptr, nullptr, &status);
                const char* name = (status == 0 && demangled) ? demangled : mangledName;
#else
                const char* name = mangledName;
#endif //__clang__
                size_t componentSize = sizeof(T);
                size_t componentAlign = alignof(T);
                size_t totalArraySize = sizeof(T) * MAX_ENTITIES;

                bool isCacheAligned = (componentSize % 64 == 0) && (componentAlign >= 64);
                ILEN_WARNING_P("COMPONENT_ARRAY", "[%s] size=%zu bytes | align=%zu | array=%zu KB | cache-aligned=%s",
                    name,
                    componentSize,
                    componentAlign,
                    totalArraySize / 1024,
                    isCacheAligned ? "SI" : "NO — posible false sharing en paralelo");

                if(!isCacheAligned){
                    size_t nextMultiple = ((componentSize + 63) / 64) * 64;
                    ILEN_ERROR_P("COMPONENT_ARRAY",
                        "[%s] Para evitar false sharing añade alignas(64) al struct "
                        "y/o paddea hasta %zu bytes (actualmente %zu)",
                        name, nextMultiple, componentSize
                    );
                }
#if defined(__GNUG__) || defined(__clang__)
                free(demangled);
#endif
            }
#endif

            ComponentArray(){
                entityToIndexMap.fill(INVALID_INDEX);
                indexToEntityMap.fill(NULL_ENTITY);

#if ILEN_DEBUG
                //PrintDiagnostics();
#endif
            }

            void insertData(Entity entity, T&& component) {
                //assert(entityToIndexMap.count(entity) == 0 && "Ya existe el componente");
                if(entity > MAX_ENTITIES){
                    ILEN_EXECUTE_ERROR_P("COMPONENT_ARRAY", "Entity ID fuera del rango permitido (overflow de entidades)");
                }
                // Si ya existe, ignorar en vez de crashear
                if(entityToIndexMap[entity] != INVALID_INDEX) return; //Existe

                size_t newIndex = size;
                entityToIndexMap[entity] = newIndex;
                indexToEntityMap[newIndex] = entity;
                componentArray[newIndex] = std::move(component);
                size++;
            }

            void removeData(Entity entity) override {
                //assert(entityToIndexMap.find(entity) != entityToIndexMap.end() && "Tratando de remover un component no existente");
                if(entity > MAX_ENTITIES) ILEN_EXECUTE_ERROR_P("COMPONENT_ARRAY", "Entity ID fuera del rango permitido (overflow de entidades)");
                if(entityToIndexMap[entity] == INVALID_INDEX) ILEN_EXECUTE_ERROR_P("COMPONENT_ARRAY", "Se esta tratando de borrar una entidad inexistente");

                size_t indexOfRemoved = entityToIndexMap[entity];
                size_t indexOfLast = size - 1;
                
                //Mantiene el array denso.
                componentArray[indexOfRemoved] = std::move(componentArray[indexOfLast]);

                Entity entityOfLast = indexToEntityMap[indexOfLast];
                entityToIndexMap[entityOfLast] = indexOfRemoved;
                indexToEntityMap[indexOfRemoved] = entityOfLast;

                //entityToIndexMap.erase(entity);
                //indexToEntityMap.erase(indexOfLast);
                entityToIndexMap[entity] = INVALID_INDEX;
                indexToEntityMap[indexOfLast] = NULL_ENTITY;
                size--;
            }

            void updateData(Entity entity, T&& component){
                if(entity > MAX_ENTITIES) ILEN_EXECUTE_ERROR_P("COMPONENT_ARRAY", "Entity ID fuera del rango permitido (overflow de entidades)");

                size_t idx = entityToIndexMap[entity];
                if(idx != INVALID_INDEX){
                    componentArray[idx] = std::forward<T>(component);
                } else {
                    insertData(entity, std::forward<T>(component));
                }
            }

            T* getData(Entity entity) {
                if(entity > MAX_ENTITIES) ILEN_EXECUTE_ERROR_P("COMPONENT_ARRAY", "Entity ID fuera del rango permitido (overflow de entidades)");

                size_t idx = entityToIndexMap[entity];
                if(idx == INVALID_INDEX) return nullptr;
                return &componentArray[idx];

                /*if (entityToIndexMap.find(entity) == entityToIndexMap.end()) {
                    return nullptr;
                }

                return &componentArray.at(entityToIndexMap.at(entity));*/
            }

            void entityDestroyed(Entity entity) override {
                /*if(entityToIndexMap.find(entity) != entityToIndexMap.end()){
                    removeData(entity);
                }*/
               if(entity < MAX_ENTITIES && entityToIndexMap[entity] != INVALID_INDEX)
                removeData(entity);
            }

            bool hasEntity(Entity entity) override {
                return entity < MAX_ENTITIES && entityToIndexMap[entity] != INVALID_INDEX;
                //return entityToIndexMap.find(entity) != entityToIndexMap.end();
            }

            T* getRawData() { return componentArray.data(); }
            size_t getActiveCount() const { return size; }
            Entity getEntityAt(size_t index){
                if(index > size) ILEN_FATAL_ERROR_P("COMPONENT_ARRAY", "Indice fuera del rango al solicitar entidad");
                return indexToEntityMap[index];
            }
        private:
            static constexpr size_t INVALID_INDEX = std::numeric_limits<size_t>::max();

            alignas(64) std::array<T, MAX_ENTITIES> componentArray;
            std::array<size_t, MAX_ENTITIES> entityToIndexMap;
            std::array<Entity, MAX_ENTITIES> indexToEntityMap;
            
            //std::unordered_map<Entity, size_t> entityToIndexMap;
            //std::unordered_map<size_t, Entity> indexToEntityMap;
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
            void setComponent(Entity entity, T&& component){
                using DecayedType = typename std::decay<T>::type;
                registerComponent<DecayedType>();

                auto arrayPtr = std::static_pointer_cast<ComponentArray<DecayedType>>(
                    componentArrays[std::type_index(typeid(DecayedType))]
                );
                arrayPtr->updateData(entity, std::forward<T>(component));
            }

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

            //Retorna la referencia al arreglo que posee todos los componentes de un mismo tipo
            template <typename T>
            ComponentArray<typename std::decay<T>::type>* getComponentArray() {
                using DecayedType = typename std::decay<T>::type;
                std::type_index typeIndex(typeid(DecayedType));

                if(componentArrays.find(typeIndex) == componentArrays.end()){
                    //Tipo de componente no registrado.
                    return nullptr;
                }

                auto genericArrayPtr = componentArrays.at(typeIndex);
                return std::static_pointer_cast<ComponentArray<DecayedType>>(genericArrayPtr).get();
            }

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