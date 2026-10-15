#pragma once

#include <cstddef>
#include <vector>
#include <functional>
#include <utility>
#include <mutex>
#include <deque>
#include <lynx/headers/lynx_logs.h>
#include <lynx/ecs/types.h>

namespace lynx::job_system {
    class CommandBuffer {
        public:
            explicit CommandBuffer(ecs::Entity maxEntities) {
                m_commands.reserve(maxEntities);
            }

            CommandBuffer(const CommandBuffer&) = delete;
            CommandBuffer& operator=(const CommandBuffer&) = delete;

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
            void Init(size_t numThreads, ecs::Entity maxEntities) {
                m_ThreadBuffers.clear();

                for(std::size_t index = 0; index < numThreads; index++) {
                    m_ThreadBuffers.emplace_back(maxEntities);
                }
            }

            CommandBuffer& getBuffer(size_t threadIndex) {
                if(threadIndex >= m_ThreadBuffers.size()){
                    LYNX_EXECUTE_ERROR_P("COMMAND_BUFFER", "Un hilo no registrado pidio una cola de trabajo.");
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