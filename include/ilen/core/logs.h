#pragma once
#include <iostream>
#include <string>
#include <ilen/core/exceptions.h>
#include <mutex>

namespace ilen::core{
    class Debug {
        public:
            static void Log(std::string content, std::string prefijo = "");
            static void Warning(std::string content, std::string prefijo = "");
            static void Error(std::string content, std::string prefijo = "");
            static void ExecuteError(std::string content, std::string prefijo = "");
            static void FatalError(const char* content, const char* prefijo = "");
        private:
            Debug() = delete;
            Debug(const Debug&) = delete;
            Debug& operator=(const Debug&) = delete;
            static std::mutex logMutex;
    };
}