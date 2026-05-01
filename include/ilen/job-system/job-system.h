// Job system for Ilusion's Engine
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
#include <ilen/headers/ilen-logs.h>

namespace ilen::job_system {

using Job = std::function<void()>;

// Grupo de sincronización liviano para sistemas ECS.
// Reemplaza std::future cuando solo necesitas saber que un batch terminó,
// sin necesitar el valor de retorno de cada job.
struct JobGroup {
    std::atomic<int> pendingJobs{0};

    // Spin-wait: eficiente cuando los jobs son cortos (sistemas ECS por frame).
    // No hace syscall al kernel, a diferencia de future::get().
    void Wait() {
        while (pendingJobs.load(std::memory_order_acquire) > 0)
            std::this_thread::yield();
    }
};

class JobSystem final {
public:
    // Inicializa el job system.
    void Initializate() {
#if ILEN_SINGLECORE_TEST
        const uint num_threads = 1;
#else
        const uint hw = std::thread::hardware_concurrency();
        const uint num_threads = (hw > 1) ? hw - 1 : 1;
#endif
        workThreads.reserve(num_threads);
        for (uint i = 0; i < num_threads; i++) {
            workThreads.emplace_back([this] { WorkLoop(); });
        }
    }

    // Detiene todos los hilos y el JobSystem (debe llamarse solo al cerrar el motor).
    void StopJobSystem() {
        shouldStop = true;
        condition.notify_all();
        for (auto& thread : workThreads) {
            if (thread.joinable())
                thread.join();
        }
    }

    // SubmitJob para sistemas ECS — sin future, sin heap alloc extra.
    // Usar cuando solo necesitas saber que el batch terminó (la mayoría de casos).
    void SubmitJob(Job job, JobGroup* group = nullptr) {
        if(!job){
            ILEN_WARNING_P(DEBUG_PREFIX, "Se encolo una tarea vacia. Favor de revisar la razón de porque sucedio esto");
            return;
        }

        bool needsNotify = false;

        if (group)
        {
            group->pendingJobs.fetch_add(1, std::memory_order_relaxed);

            Job wrappedJob = [job = std::move(job), group]() {
                job();
                group->pendingJobs.fetch_sub(1, std::memory_order_release);
            };
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                if (shouldStop)
                    throw std::runtime_error("enqueuing on stopped JobSystem");
                needsNotify = jobQueue.empty();
                jobQueue.push(std::move(wrappedJob));
            }
        }else {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                if (shouldStop) throw std::runtime_error("stopped");
                needsNotify = jobQueue.empty();
                jobQueue.push(std::move(job));
            }
        }
        if(needsNotify) condition.notify_one();
    }

    // SubmitJob con future — conservar para jobs que necesitan retornar un valor.
    template<class F, class... Args>
    auto SubmitJobWithResult(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [f = std::forward<F>(f),
             ...args = std::forward<Args>(args)]() mutable {
                return f(std::forward<Args>(args)...);
            }
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (shouldStop)
                throw std::runtime_error("enqueuing on stopped JobSystem");
            jobQueue.push([task] { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    size_t AvailableThreads() const {
        return workThreads.size();
    }

private:
    std::vector<std::thread>   workThreads;
    std::queue<Job>            jobQueue;
    std::mutex                 queueMutex;
    std::condition_variable    condition;
    std::atomic<bool>          shouldStop = false;

    const static inline char* DEBUG_PREFIX = "JOB_SYSTEM";

    static constexpr int SPIN_COUNT = 0;

    void WorkLoop() {
        while (!shouldStop) {
            Job job;

            //Fase 1 con un ciclo activo
            bool found = false;
            for(int i = 0; i < SPIN_COUNT && !shouldStop; i++){
                {
                    std::unique_lock<std::mutex> lock(queueMutex);

                    if(!jobQueue.empty()){
                        job = std::move(jobQueue.front());
                        jobQueue.pop();
                        found = true;
                        break;
                    }
                }
                std::this_thread::yield();
            }

            if(found) {
                if(!job){
                    ILEN_ERROR_P(DEBUG_PREFIX, "Llego una tarea vacia hasta el Loop Principal de los workers. Se perdio una referencia al encolar. Revisar.");
                    continue;
                }
                job();
                continue;
            }

            //Fase 2, con un Sleep real.
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, [this] {
                    return !jobQueue.empty() || shouldStop;
                });
                if (jobQueue.empty() && shouldStop)
                    return;
                job = std::move(jobQueue.front());
                jobQueue.pop();
            }

            if(!job){
                ILEN_ERROR_P(DEBUG_PREFIX, "Llego una tarea vacia hasta el Loop Principal de los workers. Se perdio una referencia al encolar. Revisar.");
                continue;
            }
            job();
        }
    }
};

} // namespace ilen::job_system