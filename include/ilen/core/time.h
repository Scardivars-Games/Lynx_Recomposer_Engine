#pragma once

#include <chrono>
#include <cassert>

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
}