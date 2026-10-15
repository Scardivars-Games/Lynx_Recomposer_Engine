#include <lynx/core/thread_context.h>

namespace lynx::core::threading {
    std::thread::id g_MainThreadId;

    thread_local size_t t_ThreadIndex = UNREGISTERED_THREAD;
    thread_local bool t_IsMainThread = false;
}