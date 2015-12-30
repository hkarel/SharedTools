/* clang-format off */
/****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализован механизм для автоматической блокировкм/разблокировки
  атомарного локкера.
  Пояснение: Spin в названии используется потому, что поток фактически
  встает в ожидание крутясь в холостом цикле while {}.

****************************************************************************/

#pragma once

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif

#include <atomic>

struct SpinLocker
{
    explicit SpinLocker(std::atomic_flag& locker) NOEXCEPT : locker(locker) {
        while (this->locker.test_and_set(std::memory_order_acquire)) {}
    }
    ~SpinLocker() NOEXCEPT {
        locker.clear(std::memory_order_release);
    }
    std::atomic_flag& locker;

    SpinLocker() = delete;
    SpinLocker(const SpinLocker&) = delete;
    SpinLocker& operator= (const SpinLocker&) = delete;
};

