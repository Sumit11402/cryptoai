#include "core/ThreadPool.hpp"
#include "core/Logger.hpp"

namespace crypto {

ThreadPool& ThreadPool::instance() {
    static ThreadPool s_instance;
    return s_instance;
}

ThreadPool::ThreadPool() {
    init(0);
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::init(size_t numThreads) {
    if (!m_workers.empty()) return;

    if (numThreads == 0) {
        numThreads = std::max(4u, std::thread::hardware_concurrency());
    }

    m_stop.store(false);
    m_workers.reserve(numThreads);

    for (size_t i = 0; i < numThreads; ++i) {
        m_workers.emplace_back([this, i]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->m_queueMutex);
                    this->m_cv.wait(lock, [this]() {
                        return this->m_stop.load() || !this->m_tasks.empty();
                    });

                    if (this->m_stop.load() && this->m_tasks.empty()) {
                        return;
                    }

                    task = std::move(this->m_tasks.front());
                    this->m_tasks.pop();
                }

                m_activeThreads.fetch_add(1);
                try {
                    task();
                } catch (const std::exception& e) {
                    LOG_ERROR("Worker thread exception: " + std::string(e.what()));
                } catch (...) {
                    LOG_ERROR("Unknown worker thread exception.");
                }
                m_activeThreads.fetch_sub(1);
            }
        });
    }

    LOG_INFO("ThreadPool initialized with " + std::to_string(numThreads) + " worker threads.");
}

void ThreadPool::shutdown() {
    if (m_stop.load()) return;

    m_stop.store(true);
    m_cv.notify_all();

    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
    LOG_INFO("ThreadPool cleanly shutdown.");
}

void ThreadPool::postToMainThread(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(m_mainQueueMutex);
    m_mainThreadQueue.push(std::move(callback));
}

void ThreadPool::processMainThreadCallbacks(size_t maxPerFrame) {
    size_t processed = 0;
    while (processed < maxPerFrame) {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(m_mainQueueMutex);
            if (m_mainThreadQueue.empty()) break;
            callback = std::move(m_mainThreadQueue.front());
            m_mainThreadQueue.pop();
        }
        if (callback) {
            try {
                callback();
            } catch (const std::exception& e) {
                LOG_ERROR("Main thread callback exception: " + std::string(e.what()));
            }
        }
        processed++;
    }
}

size_t ThreadPool::getPendingTaskCount() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_tasks.size();
}

} // namespace crypto
