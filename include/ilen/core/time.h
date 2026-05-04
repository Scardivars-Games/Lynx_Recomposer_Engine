#pragma once

#include <chrono>
#include <cassert>
#include <ilen/headers/ilen-logs.h>

namespace ilen::time{
    class Time {
        public:
            Time() = delete;

            static void Init(){
                s_startTime = clock::now();
                s_lastFrame = s_startTime;
                isInitialized = true;
            }
            static float getTotalTime(){
                isTimeInitialized();
                auto now = clock::now(); 
                std::chrono::duration<float> total = now - s_startTime;
                return total.count();
            }
            static float getDeltaTime(){
                isTimeInitialized();
                return s_deltaTime;
            }
            static float getTimeSinceCurrentFrameStart() {
                isTimeInitialized();
                auto now = clock::now();
                std::chrono::duration<float> total = now - s_lastFrame;
                return total.count();
            }
            static void setNewFrame(){
                isTimeInitialized();
                auto now = clock::now();
                std::chrono::duration<float> delta = now - s_lastFrame;
                s_deltaTime = delta.count();
                s_lastFrame = now;
            }

            static std::chrono::duration<float> getSleepTime(float targetFramerate) {
                isTimeInitialized();
                std::chrono::duration<float> framerateDuration(targetFramerate);
                auto now = clock::now();
                std::chrono::duration<float> delta = now - s_lastFrame;
                return framerateDuration - delta;
            }

            static std::chrono::high_resolution_clock::time_point getWakeTime(float targetFramerate){
                isTimeInitialized();
                std::chrono::duration<float> framerateDuration(targetFramerate);
                return s_lastFrame + std::chrono::duration_cast<clock::duration>(framerateDuration);
            }
            
            static void isTimeInitialized(){
                assert(isInitialized != false && "Se requiere Incializar el tiempo para poder hacer uso de este en el motor");
            }
        private:
            using clock = std::chrono::high_resolution_clock;
            static inline clock::time_point s_startTime;
            static inline clock::time_point s_lastFrame;
            static inline bool isInitialized = false;
            static inline float s_deltaTime = 0.0f;
    };

#if !ILEN_DISABLE_BASIC_LOGGIN
    struct ScopedTimer {
        const char* name;
        std::chrono::time_point<std::chrono::high_resolution_clock> start;

        ScopedTimer(const char* name) : name(name) {
            start = std::chrono::high_resolution_clock::now();
        }

        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto startUs = std::chrono::time_point_cast<std::chrono::microseconds>(start).time_since_epoch().count();
            auto endUs = std::chrono::time_point_cast<std::chrono::microseconds>(end).time_since_epoch().count();
            
            auto durationUs = endUs - startUs;
            float ms = durationUs / 1000.0f;
            
            ILEN_LOG_P("PROFILER", "%s: %.4f ms\n", name, ms);
            //std::cout << "[Profiler] " << name << ": " << ms << " ms\n";
        }
    };
#else
    struct ScopedTimer{
           ScopedTimer(const char* name) {}; //Solo por compatibilidad 
    };
#endif
}