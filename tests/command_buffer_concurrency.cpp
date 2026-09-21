#include <ilen/job_system/command_buffers.h>
#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

int main() {
    ilen::job_system::CommandBuffer buffer;
    constexpr int producers = 4, perProducer = 20000;
    std::vector<int> seen(producers * perProducer);
    std::atomic<int> completed = 0;
    std::vector<std::thread> threads;
    const auto consumer = std::this_thread::get_id();
    for (int p = 0; p < producers; ++p) {
        threads.emplace_back([&, p] {
            for (int i = 0; i < perProducer; ++i) {
                buffer.push([&, index = p * perProducer + i] {
                    assert(std::this_thread::get_id() == consumer);
                    ++seen[index];
                });
                if (i % 17 == 0) std::this_thread::yield();
            }
            ++completed;
        });
    }
    while (completed != producers) buffer.execute();
    for (auto& thread : threads) thread.join();
    buffer.execute();
    for (auto count : seen) assert(count == 1);
    int nested = 0;
    buffer.push([&] { ++nested; buffer.push([&] { ++nested; }); });
    buffer.execute();
    assert(nested == 1);
    buffer.execute();
    assert(nested == 2);
    // Mutex-containing buffers can still be provisioned by the dispatcher.
    ilen::job_system::CommandBufferDispatcher dispatcher;
    dispatcher.Init(4);
    dispatcher.ExecuteAll();
    std::cout << "Passed: 80000 commands exactly once on consumer; nested enqueue survives.\n";
}
