#pragma once
#include <atomic>

/**
  Сервисная структрура, используется для автоматической блокировкм/разблокировки
  атомарного локкера.
  Пояснение: Spin в названии используется потому, что поток фактически
  встает в ожидание крутясь в холостом цикле while {}.
*/
struct SpinLocker
{
    explicit SpinLocker(std::atomic_flag& locker) noexcept : locker(locker) {
        while (this->locker.test_and_set(std::memory_order_acquire)) {}
    }
    ~SpinLocker() noexcept {
        locker.clear(std::memory_order_release);
    }
    std::atomic_flag& locker;

    SpinLocker() = delete;
    SpinLocker(const SpinLocker&) = delete;
    SpinLocker& operator= (const SpinLocker&) = delete;
};

