#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <memory>

namespace crypto {

class ThreadPool {
public:
    static ThreadPool& instance();

    void init(size_t numThreads = 0);
    void shutdown();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (m_stop.load()) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_cv.notify_one();
        return res;
    }

    // UI Thread synchronization
    void postToMainThread(std::function<void()> callback);
    void processMainThreadCallbacks(size_t maxPerFrame = 50);

    size_t getActiveThreadCount() const { return m_activeThreads.load(); }
    size_t getPendingTaskCount();

private:
    ThreadPool();
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stop{false};
    std::atomic<size_t> m_activeThreads{0};

    // Main thread result queue
    std::queue<std::function<void()>> m_mainThreadQueue;
    std::mutex m_mainQueueMutex;
};

} // namespace crypto
