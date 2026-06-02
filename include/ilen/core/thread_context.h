#pragma once

#include <thread>
#include <cstddef>

namespace ilen::core::threading {
    //ID global del hilo principal
    extern std::thread::id g_MainThreadId;

    //Id, local a cada hilo, de su propio indice. Al ser local del hilo, cada hilo ve un valor
    //único para el.
    extern thread_local size_t t_ThreadIndex;
    extern thread_local bool t_IsMainThread;
    
    //Esto es por si un hilo no es creado por el jobSystem, descartarlo
    constexpr size_t UNREGISTERED_THREAD = static_cast<size_t>(-1);
}