//Job system for Ilusion's Engine
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <queue>
#include <utility>
#include <future>
#include <vector>


namespace ilen::job_system {
    using Job = std::function<void()>;

    class JobSystem {
        public:
            //Inicializa el job system.
            void Initializate() {
                const int num_threads = std::thread::hardware_concurrency() - 1;

                for(int i = 0; i< num_threads; i++) {
                    workThreads.emplace_back([this] {
                        WorkLoop();
                    });
                }
            }

            //Detiene todos los hilos y el JobSystem (debe llamarse solo al cerrar el motor)
            void StopJobSystem() {
                shouldStop = true;

                condition.notify_all();

                for(auto& thread : workThreads){
                    if(thread.joinable()){
                        thread.join();
                    }
                }
            }

            //Añade una nueva función a la cola para ser ejecutada.
            template<class F, class... Args>
            auto SubmitJob(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
            {
                using return_type = decltype(f(args...));
            
                auto task = std::make_shared< std::packaged_task<return_type()> >(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                );
            
                std::future<return_type> res = task->get_future();
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
            
                    // No encolar trabajos si el sistema se está deteniendo
                    if(shouldStop)
                        throw std::runtime_error("enqueuing on stopped JobSystem");
            
                    jobQueue.emplace([task](){ (*task)(); });
                }
                condition.notify_one();
                return res;
            }
        private:
            std::vector<std::thread> workThreads; //Lista de todos los hilos de trabajo creados por el motor.
            std::queue<Job> jobQueue; //Queue con todos los trabajos de la app.
            std::mutex queueMutex; //Mutex del queue para que pueda ser administrado de forma segura por diferentes hilos.
            std::condition_variable condition; //Se encarga de dormir y despertar hilos.
            std::atomic<bool> shouldStop = false; //Detiene el sistema por completo. 
            
            //Loop que ejecutan los hilos disponibilizados para el apoyo.
            void WorkLoop() {
                while(true) {
                    Job job;
                    //Mutex region
                    {   
                        std::unique_lock<std::mutex> lock(queueMutex);

                        condition.wait(lock, [this] {
                            return !jobQueue.empty() || shouldStop;
                        });

                        if(jobQueue.empty() && shouldStop){
                            return; //Rompemos el ciclo, libera el hilo ya para siempre en el resto de la ejecución del motor.
                        }

                        job = std::move(jobQueue.front());
                        jobQueue.pop();
                    }

                    job();
                }
            }
    };
}