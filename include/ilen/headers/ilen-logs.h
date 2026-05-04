#pragma once

#include <ilen/core/logs.h>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#if ILEN_WINDOWS
    #define __ILEN_FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #define __ILEN_FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif


//Macros para el loggin
#if !ILEN_DISABLE_BASIC_LOGGIN
    #define ILEN_LOG(format, ...) \
        do { \
            char prefixBuffer[1024]; \
            snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __ILEN_FILENAME__, __LINE__); \
            ilen::internal::Debug::Log(format, prefixBuffer, ##__VA_ARGS__); \
        } while (0)
    #define ILEN_LOG_P(prefix, format, ...) \
        do { \
            char prefixBuffer[1024]; \
            snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __ILEN_FILENAME__, __LINE__); \
            ilen::internal::Debug::Log(format, prefixBuffer, ##__VA_ARGS__); \
        } while (0)
#else
    #define ILEN_LOG(...)
    #define ILEN_LOG_P(...)
#endif

#define ILEN_WARNING(format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __ILEN_FILENAME__, __LINE__); \
        ilen::internal::Debug::Warning(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define ILEN_ERROR(format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __ILEN_FILENAME__, __LINE__); \
        ilen::internal::Debug::Error(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define ILEN_EXECUTE_ERROR(format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __ILEN_FILENAME__, __LINE__); \
        ilen::internal::Debug::ExecuteError(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define ILEN_FATAL_ERROR(format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __ILEN_FILENAME__, __LINE__); \
        ilen::internal::Debug::FatalError(format, prefixBuffer); \
    } while (0)


//Set con prefijo personalizado!!!!
    
#define ILEN_WARNING_P(prefix, format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __ILEN_FILENAME__, __LINE__); \
        ilen::internal::Debug::Warning(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define ILEN_ERROR_P(prefix, format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __ILEN_FILENAME__, __LINE__); \
        ilen::internal::Debug::Error(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define ILEN_EXECUTE_ERROR_P(prefix, format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __ILEN_FILENAME__, __LINE__); \
        ilen::internal::Debug::ExecuteError(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define ILEN_FATAL_ERROR_P(prefix, format, ...) do { char prefixBuffer[1024]; snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __ILEN_FILENAME__, __LINE__); ilen::internal::Debug::FatalError(format, prefixBuffer, ##__VA_ARGS__); } while (0)