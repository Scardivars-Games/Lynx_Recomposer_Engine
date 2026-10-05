#pragma once

#include <vector>
#include <functional>
#include <utility>
#include <mutex>
#include <deque>
#include <ilen/headers/ilen_logs.h>
#include <ilen/ecs/types.h>

namespace ilen::job_system {
    class CommandBuffer {
        public:
            CommandBuffer() {
                m_commands.reserve(ilen::ecs::MAX_ENTITIES);
            }

            void push(std::function<void()> command) {
                std::lock_guard lock(m_mutex);
                m_commands.emplace_back(std::move(command));
            }

            void execute() {
                std::vector<std::function<void()>> pending;
                {
                    std::lock_guard lock(m_mutex);
                    pending.swap(m_commands);
                }
                for (auto& command : pending) command();

            }

        private:
            std::mutex m_mutex;
            std::vector<std::function<void()>> m_commands;
    };

    class CommandBufferDispatcher {
        public: 
            void Init(size_t numThreads) {
                m_ThreadBuffers.resize(numThreads);
            }

            CommandBuffer& getBuffer(size_t threadIndex) {
                if(threadIndex >= m_ThreadBuffers.size()){
                    ILEN_EXECUTE_ERROR_P("COMMAND_BUFFER", "Un hilo no registrado pidio una cola de trabajo.");
                }
                return m_ThreadBuffers[threadIndex];
            }

            void ExecuteAll() {
                for(auto& buffer: m_ThreadBuffers){
                    buffer.execute();
                }
            }
        private:
            std::deque<CommandBuffer> m_ThreadBuffers;
    };
}