#pragma once
#include <thread>

// Provides a compatibility alias for `std::jthread`.
// When `std::jthread` is unavailable, a minimal wrapper around `std::thread`
// is supplied that joins on destruction but lacks stop token support.
#if defined(__cpp_lib_jthread)
#include <stop_token>
namespace th_compat {
using jthread = std::jthread;
}
#else
namespace th_compat {
class jthread {
    std::thread t;

  public:
    jthread() noexcept = default;
    template <class Fn, class... Args>
    explicit jthread(Fn&& fn, Args&&... args)
        : t(std::forward<Fn>(fn), std::forward<Args>(args)...) {}
    jthread(jthread&&) noexcept = default;
    jthread& operator=(jthread&&) noexcept = default;
    ~jthread() {
        if (t.joinable())
            t.join();
    }
    void join() {
        if (t.joinable())
            t.join();
    }
    bool joinable() const noexcept { return t.joinable(); }
};
} // namespace th_compat
#endif
