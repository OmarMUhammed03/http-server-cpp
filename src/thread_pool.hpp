#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <condition_variable>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include "constants.hpp"

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = constants::THREAD_POOL_SIZE);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Returns true if enqueued, false if queue full (spike >10k -> reject)
    bool try_enqueue(std::function<void()> task);
    // Throws or blocks? For compatibility - delegates to try_enqueue
    void enqueue(std::function<void()> task);

    void shutdown();
    size_t queue_size() const;
    size_t rejected_count() const noexcept { return rejected_.load(); }

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> tasks_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> rejected_{0};
};

#endif
