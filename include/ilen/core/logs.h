#pragma once
#include <iostream>
#include <string>
#include <ilen/core/exceptions.h>
#include <mutex>

namespace ilen::internal{
    class Debug {
        public:
            static void Log(const char* content, const char* prefijo = "", ...);
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