#pragma once

#include <lynx/ecs/types.h>
#include <lynx/ecs/system.h>
#include <lynx/headers/lynx_logs.h>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <map>
#include <algorithm>

namespace lynx::ecs {
    //Estructura para ordenar el orden de los sistemas.
    struct SystemConfig {
        std::shared_ptr<ISystem> system;
        int priority;
    };

    class SystemManager {
        public:
            template<typename T>
            std::shared_ptr<T> registerSystem(SystemPhase phase, int priority) {
                const char* typeName = typeid(T).name();
                if(systems.find(typeName) != systems.end()){
                    std::string error = std::string("Sistema ya registrado: ") + typeName;
                    LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, error.c_str());
                }

                auto system = std::make_shared<T>();
                systems.insert({typeName, system});
                systemExecutionOrder[phase].push_back({system, priority});
                LYNX_LOG_P(DEBUG_PREFIX, "El sistema %s fue registrado de manera exitosa.", typeName);
                return system;
            };//Registra un nuevo sistema y regresa su puntero

            template<typename T>
            void setSignature(Signature signature) {
                const char* typeName = typeid(T).name();
                assert(systems.find(typeName) != systems.end() && "Usando un sistema no registrado");
                signatures.insert({typeName, signature});
            }; //Asigna una firma al sistema

            //Obtiene un sistema ya registrado
            template<typename T>
            std::shared_ptr<T> getSystem() {
                const char* typeName = typeid(T).name();
                auto it = systems.find(typeName);

                if(it != systems.end()){
                    //Encontro el sistema
                    return std::static_pointer_cast<T>(it->second);
                }
                LYNX_WARNING_P(DEBUG_PREFIX, "El sistema %s no fue encontrado. Favor de revisar si se registro antes de usarlo.", typeName);
                return nullptr; //No encontro el Sistema
            };

            void entityDestroyed(Entity entity); //Notifica a los sistema que la entidad ha sido destruida
            
            //A llamar cuando la firma de una entidad cambia
            //Comprueba que sistemas ahora deben o no procesar a esta entidad
            void entitySignatureChanged(Entity entity, Signature newSignature); 

            //Esto es para organizar los sistemas y se puedan ejecutar en orden
            void sortSystems();

            std::vector<std::shared_ptr<ISystem>> getSystemByPhase(SystemPhase phase);

            //Retorna todos los sistemas registrador para iterar en ellos.
            std::vector<std::shared_ptr<ISystem>> getAllSystems();

        private:
            std::unordered_map<const char*, Signature> signatures; //Mapa de nombre de tipo a firmas de sistema.
            
            std::unordered_map<const char*, std::shared_ptr<lynx::ecs::ISystem>> systems; //Mapa de nombre a instancias de los sistemas
            std::map<SystemPhase, std::vector<SystemConfig>> systemExecutionOrder; //Mapa ordenado con el orden de ejecucion de los sistemas.
            static inline const char* DEBUG_PREFIX = "SYSTEM_MANAGER";
    };
}