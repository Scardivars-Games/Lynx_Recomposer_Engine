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
        if (group)
            group->pendingJobs.fetch_add(1, std::memory_order_relaxed);
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (shouldStop)
                throw std::runtime_error("enqueuing on stopped JobSystem");
            jobQueue.push(std::move(job));
        }
        condition.notify_one();
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

    void WorkLoop() {
        while (true) {
            Job job;
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
            job();
        }
    }
};

} // namespace ilen::job_system