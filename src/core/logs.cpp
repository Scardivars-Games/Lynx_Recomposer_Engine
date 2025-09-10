#include <ilen/core/logs.h>

namespace ilen::core{
    std::string preparatePrefix(std::string prefijo){
        if(prefijo != ""){
            return " (" + prefijo + ")";
        }

        return "";
    }

    void Debug::Log(std::string content, std::string prefijo){
        std::cout << "\033[32m[DEBUG"<< preparatePrefix(prefijo) << ":] \033[0m" << content << std::endl;
    }

    void Debug::Warning(std::string content, std::string prefijo) {
        std::clog << "\033[33m[WARNING"<< preparatePrefix(prefijo) << "] \033[0m" << content << std::endl;
    }

    void Debug::Error(std::string content, std::string prefijo) {
        std::cerr << "\033[31m[ERROR"<< preparatePrefix(prefijo) << ":] \033[0m" << content << std::endl;
    }

    void Debug::ExecuteError(std::string content, std::string prefijo){
        throw BasicException(content, prefijo);
    }

    void Debug::FatalError(const char* content, const char* prefijo){
        std::cerr << "\033[31m [FATAL_ERROR"<< preparatePrefix(std::string(prefijo)) << "] \033[0m" << content << std::endl;
    }
}