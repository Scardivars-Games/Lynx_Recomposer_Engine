#pragma once
#include <fstream>
#include <vector>
#include <filesystem>
#include <sstream>
#include <cassert>
#include <ilen/headers/enumerations.h>

using namespace ilen::enumerations;

namespace ilen::core{
    class FileManager {
        public:
            static void Init();
            static std::string readFile(const std::string& filename, ILEN_FILE_TYPE category);
            static std::filesystem::path fileRoute(const std::string& filename, ILEN_FILE_TYPE category);
            static std::filesystem::path getApplicationWrittablePath() {                 
                checkIfInited();
                return s_applicationWrittablePath; 
            };
            static std::filesystem::path getApplicationResourcesPath() {
                checkIfInited();
                return s_applicationResourcesPath;
            };
            static void checkIfInited(){
                assert(isInited != false && "No se puede usar los métodos de FileManager sin llamar a Init");
            };

        private:
            const static inline std::string DEBUG_PREFIX = "FILEMANAGER";
            static inline bool isInited = false;
            static std::string filePreRoute(ILEN_FILE_TYPE category);
            static std::filesystem::path s_applicationWrittablePath;
            static std::filesystem::path s_applicationResourcesPath;
    };
}