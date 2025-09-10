#pragma once

#include <ilen/ecs/types.h>
#include <ilen/ecs/system.h>
#include <memory>
#include <unordered_map>
#include <cassert>


namespace ilen::ecs {
    class SystemManager {
        public:
            template<typename T>
            std::shared_ptr<T> registerSystem(){
                const char* typeName = typeid(T).name();
                assert(systems.find(typeName) == systems.end() && "Sistema ya registrado");

                auto system = std::make_shared<T>();
                systems.insert({typeName, system});
                return system;
            };//Registra un nuevo sistema y regresa su puntero

            template<typename T>
            void setSignature(Signature signature) {
                const char* typeName = typeid(T).name();
                assert(systems.find(typeName) != systems.end() && "Usando un sistema no registrado");
                signatures.insert({typeName, signature});
            }; //Asigna una firma al sistema

            void entityDestroyed(Entity entity); //Notifica a los sistema que la entidad ha sido destruida
            
            //A llamar cuando la firma de una entidad cambia
            //Comprueba que sistemas ahora deben o no procesar a esta entidad
            void entitySignatureChanged(Entity entity, Signature newSignature); 

        private:
            std::unordered_map<const char*, Signature> signatures; //Mapa de nombre de tipo a firmas de sistema.
            
            std::unordered_map<const char*, std::shared_ptr<System>> systems; //Mapa de nombre a instancias de los sistemas
    };
}