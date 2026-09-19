#include "thread_pool.hpp"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads) {
    if (num_threads == 0) num_threads = 4;
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    bool expected = false;
    if (!stop_.compare_exchange_strong(expected, true)) {
        return; // already stopped
    }
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

bool ThreadPool::try_enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_.load()) return false;
        if (tasks_.size() >= constants::MAX_QUEUE_SIZE) {
            rejected_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
    return true;
}

void ThreadPool::enqueue(std::function<void()> task) {
    // unbounded compatibility wrapper - still bounded, returns false->logs
    if (!try_enqueue(std::move(task))) {
        std::cerr << "ThreadPool queue full (" << constants::MAX_QUEUE_SIZE
                  << ") - task rejected\n";
    }
}

size_t ThreadPool::queue_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stop_.load() || !tasks_.empty(); });
            if (stop_.load() && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "ThreadPool task exception: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "ThreadPool task unknown exception\n";
        }
    }
}
