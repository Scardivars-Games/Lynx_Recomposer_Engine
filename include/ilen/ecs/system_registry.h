#pragma once

//Este código definirá macros para el registro automatico de sistemas en el motor. Esta pensado para el uso de los sistemas internos, mientras que los externos (definidos por el propio juego del usuario) 
//usaran otra forma menos invasiva como un JSON con los datos y construir un código para eso.

#include <functional>
#include <vector>
#include <memory>
#include <ilen/ecs/system.h>
#include <ilen/core/engine-context.h>
#include <ilen/ecs/ecs.h>

using SystemPhase = ilen::ecs::SystemPhase;

namespace ilen::ecs {
    using SystemRegistrationCallback = std::function<void(ilen::EngineContext*)>;

    class SystemRegistry {
        public:
            static std::vector<SystemRegistrationCallback>& GetRegistrators(){
                static std::vector<SystemRegistrationCallback> registrators;
                return registrators;
            }
    };

    //Funcion auxiliar para construir la firma de los sistemas.
    template<typename SystemClass, typename... ComponentClasses>
    void ApplySignature(ilen::EngineContext* context){
        Signature signature;//Firma limpia

        if constexpr (sizeof...(ComponentClasses) > 0){
            (signature.set(context->p_coordinator->getComponentType<ComponentClasses>(), true), ...);
        }

        context->p_coordinator->setSystemSignature<SystemClass>(signature);
    }

    //Parafernalia para evitar que los sistemas se dupliquen en su cabecera (el macro que los registra)
    template<typename T>
    struct SystemRegistrar{};

    template<typename T>
    struct SystemSignature{};
}


//Para poder unir los datos sin que el compilador se haga bola
#define ILEN_CONCAT_IMPL(x, y) x##y
#define ILEN_CONCAT(x, y) ILEN_CONCAT_IMPL(x, y)

//Macro 1: Definir el sistemas
#define REGISTER_NATIVE_SYSTEM(SystemClass, Phase, Priority) \
    template<> struct ilen::ecs::SystemRegistrar<SystemClass> { \
        SystemRegistrar() {\
            static std::once_flag flag; \
            std::call_once(flag, []{\
                ilen::ecs::SystemRegistry::GetRegistrators().emplace_back(\
                    [](ilen::EngineContext* context) {\
                        context->p_coordinator->registerSystem<SystemClass>(Phase, Priority);\
                    }\
                );\
            });\
        }\
        static SystemRegistrar<SystemClass> instance;\
    };\
    inline ilen::ecs::SystemRegistrar<SystemClass> ilen::ecs::SystemRegistrar<SystemClass>::instance;

//Macro 2: Componentes requeridos por cada sistema.
#define REQUIRED_COMPONENTS_FOR_SYSTEM(SystemClass, ...) \
    template<> struct ilen::ecs::SystemSignature<SystemClass> {\
        SystemSignature() {\
            static std::once_flag flag;\
            std::call_once(flag, []{\
                ilen::ecs::SystemRegistry::GetRegistrators().emplace_back(\
                    [](ilen::EngineContext* context){\
                        ilen::ecs::ApplySignature<SystemClass __VA_OPT__(,) __VA_ARGS__>(context);\
                    }\
                );\
            });\
        }\
        static SystemSignature<SystemClass> instance;\
    };\
    inline ilen::ecs::SystemSignature<SystemClass> ilen::ecs::SystemSignature<SystemClass>::instance;