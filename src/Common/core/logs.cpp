#include <ilen/core/logs.h>
#include <ilen/window/alert-box.h>

namespace ilen::internal{
    std::mutex Debug::logMutex;

    std::string preparatePrefix(std::string prefijo){
        if(prefijo != ""){
            return " (" + prefijo + ")";
        }

        return "";
    }

    void Debug::Log(const char* content, const char* prefijo, ...){
        std::lock_guard<std::mutex> lock(logMutex);
        char messageBuffer[4096];

        va_list args;
        va_start(args, prefijo);

        vsnprintf(messageBuffer, sizeof(messageBuffer), content, args);

        va_end(args);

        std::cout << "\033[32m[DEBUG"<< preparatePrefix(prefijo) << ":] \033[0m" << messageBuffer << std::endl;
    }

    void Debug::Warning(const char* content, const char* prefijo, ...) {
        std::lock_guard<std::mutex> lock(logMutex);
                char messageBuffer[4096];

        va_list args;
        va_start(args, prefijo);

        vsnprintf(messageBuffer, sizeof(messageBuffer), content, args);
        va_end(args);
        std::clog << "\033[33m[WARNING"<< preparatePrefix(prefijo) << "] \033[0m" << messageBuffer << std::endl;
    }

    void Debug::Error(const char* content, const char* prefijo, ...) {
        std::lock_guard<std::mutex> lock(logMutex);
                char messageBuffer[4096];

        va_list args;
        va_start(args, prefijo);

        vsnprintf(messageBuffer, sizeof(messageBuffer), content, args);

        va_end(args);
        std::cerr << "\033[31m[ERROR"<< preparatePrefix(prefijo) << ":] \033[0m" << messageBuffer << std::endl;
    }

    void Debug::ExecuteError(const char* content, const char* prefijo, ...){
        char messageBuffer[4096];

        va_list args;
        va_start(args, prefijo);

        vsnprintf(messageBuffer, sizeof(messageBuffer), content, args);

        va_end(args);
        throw ilen::core::BasicException(messageBuffer, prefijo);
    }

    void Debug::FatalError(const char* content, const char* prefijo){
        std::lock_guard<std::mutex> lock(logMutex);
        std::cerr << "\033[31m [FATAL_ERROR"<< preparatePrefix(std::string(prefijo)) << "] \033[0m" << content << std::endl;
        ilen::window::box::ShowErrorDialogBox("FATAL ERROR", content);
        //throw ilen::core::BasicException(content, prefijo);
    }
}