#pragma once

#include <vector>
#include <functional>
#include <utility>
#include <ilen/headers/ilen-logs.h>
#include <ilen/ecs/types.h>

namespace ilen::job_system {
    class CommandBuffer {
        public:
            CommandBuffer() {
                m_commands.reserve(ilen::ecs::MAX_ENTITIES);
            }

            void push(std::function<void()> command) {
                m_commands.emplace_back(std::move(command));
            }

            void execute() {
                for(auto& command: m_commands){
                    command();
                }

                m_commands.clear();
            }

        private:
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
            std::vector<CommandBuffer> m_ThreadBuffers;
    };
}