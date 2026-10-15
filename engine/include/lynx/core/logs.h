#pragma once
#include <iostream>
#include <string>
#include <lynx/core/exceptions.h>
#include <mutex>
#include <stdarg.h>

namespace lynx::internal{
    class Debug {
        public:
#if !ILEN_DISABLE_BASIC_LOGGIN
            static void Log(const char* content, const char* prefijo = "", ...);
#endif
            static void Warning(const char* content, const char* prefijo = "", ...);
            static void Error(const char* content, const char* prefijo = "", ...);
            static void ExecuteError(const char* content, const char* prefijo = "", ...);
            static void FatalError(const char* content, const char* prefijo = "");
        private:
            Debug() = delete;
            Debug(const Debug&) = delete;
            Debug& operator=(const Debug&) = delete;
            static std::mutex logMutex;
    };
}