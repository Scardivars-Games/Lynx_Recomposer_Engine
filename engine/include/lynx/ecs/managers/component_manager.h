#pragma once

#include <lynx/ecs/types.h>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <cassert>
#include <utility>
#include <vector>
#include <lynx/headers/lynx_logs.h>
#if LYNX_DEBUG
#include <cxxabi.h>
#endif

namespace lynx::ecs {

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
#if LYNX_DEBUG
            static void PrintDiagnostics(Entity maxEntities){
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
                size_t totalArraySize = sizeof(T) * maxEntities;

                bool isCacheAligned = (componentSize % 64 == 0) && (componentAlign >= 64);
                LYNX_WARNING_P("COMPONENT_ARRAY", "[%s] size=%zu bytes | align=%zu | array=%zu KB | cache-aligned=%s",
                    name,
                    componentSize,
                    componentAlign,
                    totalArraySize / 1024,
                    isCacheAligned ? "SI" : "NO — posible false sharing en paralelo");

                if(!isCacheAligned){
                    size_t nextMultiple = ((componentSize + 63) / 64) * 64;
                    LYNX_ERROR_P("COMPONENT_ARRAY",
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

            explicit ComponentArray(Entity maxEntities):
                m_entityToIndexMap(maxEntities, INVALID_INDEX),
                m_indexToEntityMap(maxEntities, NULL_ENTITY),
                m_maxEntities(maxEntities)
            {
                m_components.reserve(maxEntities);
#if LYNX_DEBUG
                //PrintDiagnostics();
#endif
            }

            void validateEntity(Entity entity) const {
                if(entity >= m_maxEntities) {
                    LYNX_EXECUTE_ERROR_P("COMPONENT_ARRAY","Entity ID fuera del rango permitido");
                }
            }

            void insertData(Entity entity, T&& component) {
                //assert(entityToIndexMap.count(entity) == 0 && "Ya existe el componente");
                validateEntity(entity);
                // Si ya existe, ignorar en vez de crashear
                if(m_entityToIndexMap[entity] != INVALID_INDEX) return; //Existe

                const size_t newIndex = m_components.size();
                m_entityToIndexMap[entity] = newIndex;
                m_indexToEntityMap[newIndex] = entity;
                
                m_components.push_back(std::move(component));
            }

            void removeData(Entity entity) override {
                validateEntity(entity);

                const std::size_t removedIndex = m_entityToIndexMap[entity];

                if(removedIndex == INVALID_INDEX) {
                    LYNX_EXECUTE_ERROR_P("COMPONENT_ARRAY", "La entidad no posee este componente");
                }

                const size_t lastIndex = m_components.size() - 1;

                if(removedIndex != lastIndex) {
                    m_components[removedIndex] = std::move(m_components[lastIndex]);

                    const Entity movedEntity = m_indexToEntityMap[lastIndex];
                    m_entityToIndexMap[movedEntity] = removedIndex;
                    m_indexToEntityMap[removedIndex] = movedEntity;
                }

                m_components.pop_back();
                m_entityToIndexMap[entity] = INVALID_INDEX;
                m_indexToEntityMap[lastIndex] = NULL_ENTITY;
            }

            void updateData(Entity entity, T&& component){
                validateEntity(entity);
                size_t idx = m_entityToIndexMap[entity];
                if(idx != INVALID_INDEX){
                    m_components[idx] = std::forward<T>(component);
                } else {
                    insertData(entity, std::forward<T>(component));
                }
            }

            T* getData(Entity entity) {
                validateEntity(entity);

                size_t idx = m_entityToIndexMap[entity];
                if(idx == INVALID_INDEX) return nullptr;
                return &m_components[idx];

                /*if (entityToIndexMap.find(entity) == entityToIndexMap.end()) {
                    return nullptr;
                }

                return &componentArray.at(entityToIndexMap.at(entity));*/
            }

            void entityDestroyed(Entity entity) override {
                /*if(entityToIndexMap.find(entity) != entityToIndexMap.end()){
                    removeData(entity);
                }*/
                if(entity < m_maxEntities && m_entityToIndexMap[entity] != INVALID_INDEX)
                    removeData(entity);
            }

            bool hasEntity(Entity entity) override {
                return entity < m_maxEntities && m_entityToIndexMap[entity] != INVALID_INDEX;
                //return entityToIndexMap.find(entity) != entityToIndexMap.end();
            }

            T* getRawData() { return m_components.data(); }

            size_t getActiveCount() const { return m_components.size(); }

            Entity getEntityAt(size_t index){
                if(index >= m_components.size()) LYNX_FATAL_ERROR_P("COMPONENT_ARRAY", "Indice fuera del rango al solicitar entidad");
                    return m_indexToEntityMap[index];
            }
        private:
            static constexpr size_t INVALID_INDEX = std::numeric_limits<size_t>::max();

            std::vector<T> m_components;
            std::vector<size_t> m_entityToIndexMap;
            std::vector<Entity> m_indexToEntityMap;

            Entity m_maxEntities;
            
            //std::unordered_map<Entity, size_t> entityToIndexMap;
            //std::unordered_map<size_t, Entity> indexToEntityMap;
            //size_t m_size = 0;
    };

    class ComponentManager {
        public:
            explicit ComponentManager(Entity maxEntities) : m_maxEntities(maxEntities) {}

            template<typename T>
            void registerComponent()
            {
                using DecayedType = typename std::decay<T>::type;
                std::type_index typeIndex(typeid(DecayedType));

                if (componentTypes.find(typeIndex) != componentTypes.end()) {
                    return; //Componente ya registrado, ignorar.
                }
                componentTypes.emplace(typeIndex, nextComponentType);
                componentArrays.emplace(typeIndex, std::make_shared<ComponentArray<DecayedType>>(m_maxEntities));
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

#ifdef LYNX_DEBUG
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
            Entity m_maxEntities;
            
            std::unordered_map<std::type_index, ComponentType> componentTypes{};
            std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays{};
            ComponentType nextComponentType{};

#ifdef LYNX_DEBUG
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
#endif //LYNX_DEBUG
    };
}