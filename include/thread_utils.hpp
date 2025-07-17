#ifndef THREAD_UTILS_HPP
#define THREAD_UTILS_HPP
#include <thread>

/**
 * @brief RAII wrapper that joins the thread on destruction.
 */
struct ThreadGuard {
    std::thread t;
    ThreadGuard() = default;
    explicit ThreadGuard(std::thread&& t_) : t(std::move(t_)) {}
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
    ThreadGuard(ThreadGuard&&) = delete;
    ThreadGuard& operator=(ThreadGuard&&) = delete;
    ~ThreadGuard() {
        if (t.joinable())
            t.join();
    }
};

#endif // THREAD_UTILS_HPP
