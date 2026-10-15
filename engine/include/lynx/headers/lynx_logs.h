#pragma once

#include <lynx/core/logs.h>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#if LYNX_WINDOWS
    #define __LYNX_FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #define __LYNX_FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif


//Macros para el loggin
#if !LYNX_DISABLE_BASIC_LOGGIN
    #define LYNX_LOG(format, ...) \
        do { \
            char prefixBuffer[1024]; \
            snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __LYNX_FILENAME__, __LINE__); \
            lynx::internal::Debug::Log(format, prefixBuffer, ##__VA_ARGS__); \
        } while (0)
    #define LYNX_LOG_P(prefix, format, ...) \
        do { \
            char prefixBuffer[1024]; \
            snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __LYNX_FILENAME__, __LINE__); \
            lynx::internal::Debug::Log(format, prefixBuffer, ##__VA_ARGS__); \
        } while (0)
#else
    #define LYNX_LOG(...)
    #define LYNX_LOG_P(...)
#endif

#define LYNX_WARNING(format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __LYNX_FILENAME__, __LINE__); \
        lynx::internal::Debug::Warning(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define LYNX_ERROR(format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __LYNX_FILENAME__, __LINE__); \
        lynx::internal::Debug::Error(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define LYNX_EXECUTE_ERROR(format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __LYNX_FILENAME__, __LINE__); \
        lynx::internal::Debug::ExecuteError(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define LYNX_FATAL_ERROR(format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s:%d", __LYNX_FILENAME__, __LINE__); \
        lynx::internal::Debug::FatalError(format, prefixBuffer); \
    } while (0)


//Set con prefijo personalizado!!!!
    
#define LYNX_WARNING_P(prefix, format, ...) \
    do { \
        char prefixBuffer[2048]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __LYNX_FILENAME__, __LINE__); \
        lynx::internal::Debug::Warning(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define LYNX_ERROR_P(prefix, format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __LYNX_FILENAME__, __LINE__); \
        lynx::internal::Debug::Error(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define LYNX_EXECUTE_ERROR_P(prefix, format, ...) \
    do { \
        char prefixBuffer[1024]; \
        snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __LYNX_FILENAME__, __LINE__); \
        lynx::internal::Debug::ExecuteError(format, prefixBuffer, ##__VA_ARGS__); \
    } while (0)

#define LYNX_FATAL_ERROR_P(prefix, format, ...) do { char prefixBuffer[1024]; snprintf(prefixBuffer, sizeof(prefixBuffer), "%s (%s:%d)", prefix, __LYNX_FILENAME__, __LINE__); lynx::internal::Debug::FatalError(format, prefixBuffer, ##__VA_ARGS__); } while (0)