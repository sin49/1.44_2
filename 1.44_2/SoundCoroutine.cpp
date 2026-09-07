#include "SoundCoroutine.h"
#include <windows.h>
#include <algorithm>

namespace AudioScheduler {

    struct ScheduledCoroutine {
        uint64_t resumeTimeMs;
        std::coroutine_handle<> handle;
        uint32_t generation;
    };

    static std::mutex g_schedulerMutex;
    static std::condition_variable g_schedulerCv;
    static std::vector<ScheduledCoroutine> g_scheduledList;
    static std::atomic<bool> g_schedulerActive{ false };
    static std::atomic<uint32_t> g_currentGeneration{ 0 };
    static std::thread g_dispatcherThread;

    static uint64_t GetCurrentTimeMs() {
        return (uint64_t)GetTickCount64();
    }

    static void DispatcherLoop() {
        while (g_schedulerActive) {
            std::vector<std::coroutine_handle<>> readyHandles;
            uint32_t currentGen = g_currentGeneration.load();

            {
                std::unique_lock<std::mutex> lock(g_schedulerMutex);
                if (!g_schedulerActive) break;

                uint64_t now = GetCurrentTimeMs();

                // 만료된 작업 추출
                auto it = std::remove_if(g_scheduledList.begin(), g_scheduledList.end(),
                    [&](const ScheduledCoroutine& item) {
                        if (item.generation != currentGen) {
                            if (item.handle && !item.handle.done()) {
                                item.handle.destroy();
                            }
                            return true;
                        }
                        if (item.resumeTimeMs <= now) {
                            readyHandles.push_back(item.handle);
                            return true;
                        }
                        return false;
                    });
                g_scheduledList.erase(it, g_scheduledList.end());

                // 다음 대기 시간 계산
                if (readyHandles.empty()) {
                    if (g_scheduledList.empty()) {
                        g_schedulerCv.wait_for(lock, std::chrono::milliseconds(50));
                    } else {
                        uint64_t earliest = g_scheduledList.front().resumeTimeMs;
                        for (const auto& item : g_scheduledList) {
                            if (item.resumeTimeMs < earliest) earliest = item.resumeTimeMs;
                        }
                        int64_t waitTime = (int64_t)(earliest - now);
                        if (waitTime <= 0) waitTime = 1;
                        if (waitTime > 50) waitTime = 50;
                        g_schedulerCv.wait_for(lock, std::chrono::milliseconds((long)waitTime));
                    }
                }
            }

            // 락 해제 후 코루틴 재개 (논블로킹 실행)
            for (auto& h : readyHandles) {
                if (h && !h.done()) {
                    h.resume();
                }
            }
        }
    }

    void Initialize() {
        if (g_schedulerActive) return;
        g_schedulerActive = true;
        g_dispatcherThread = std::thread(DispatcherLoop);
    }

    void Release() {
        g_schedulerActive = false;
        CancelAll();
        g_schedulerCv.notify_all();
        if (g_dispatcherThread.joinable()) {
            g_dispatcherThread.join();
        }
    }

    void ScheduleResume(std::coroutine_handle<> handle, int delayMs) {
        if (!g_schedulerActive || !handle) return;
        uint64_t resumeTime = GetCurrentTimeMs() + (uint64_t)(max(1, delayMs));
        uint32_t gen = g_currentGeneration.load();

        {
            std::lock_guard<std::mutex> lock(g_schedulerMutex);
            g_scheduledList.push_back({ resumeTime, handle, gen });
        }
        g_schedulerCv.notify_one();
    }

    void CancelAll() {
        g_currentGeneration++;
        std::lock_guard<std::mutex> lock(g_schedulerMutex);
        for (auto& item : g_scheduledList) {
            if (item.handle && !item.handle.done()) {
                item.handle.destroy();
            }
        }
        g_scheduledList.clear();
        g_schedulerCv.notify_all();
    }

    // 모든 대기 중인 항목의 resumeTime을 "지금"으로 앞당겨서 즉시 재개시킴
    // → BGM 코루틴이 co_await 중에도 즉시 깨어나 종료 조건을 확인할 수 있음
    void WakeAll() {
        {
            std::lock_guard<std::mutex> lock(g_schedulerMutex);
            uint64_t now = GetCurrentTimeMs();
            for (auto& item : g_scheduledList) {
                item.resumeTimeMs = now; // 모든 항목을 "즉시 재개" 상태로
            }
        }
        g_schedulerCv.notify_all();
    }
}
