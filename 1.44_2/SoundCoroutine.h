#pragma once
#include <coroutine>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <cstdint>

// ==========================================================
// C++20 Coroutine Task for Audio
// ==========================================================
struct SoundTask {
    struct promise_type {
        SoundTask get_return_object() noexcept {
            return SoundTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
    };

    std::coroutine_handle<promise_type> handle;
    explicit SoundTask(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
};

// ==========================================================
// C++20 Sound Scheduler & Awaiter
// ==========================================================
namespace AudioScheduler {
    void Initialize();
    void Release();
    void ScheduleResume(std::coroutine_handle<> handle, int delayMs);
    void CancelAll();
    void WakeAll(); // 스케줄된 모든 항목을 즉시 실행 가능 상태로 전환 (BGM 즉시 중단용)
}

struct SoundDelay {
    int delayMs;
    bool await_ready() const noexcept { return delayMs <= 0; }
    void await_suspend(std::coroutine_handle<> handle) const noexcept {
        AudioScheduler::ScheduleResume(handle, delayMs);
    }
    void await_resume() const noexcept {}
};
