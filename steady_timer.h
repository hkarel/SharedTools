/* clang-format off */
/*****************************************************************************
  В модуле реализован простейший механизм для точного отсчета интервалов
  времени.

*****************************************************************************/

#pragma once

#include <ctime>
#include <chrono>

struct steady_timer
{
    typedef std::chrono::steady_clock clock;

    steady_timer() = default;
    steady_timer(const steady_timer&) = default;
    steady_timer& operator= (const steady_timer&) = default;

    // Возвращает количество прошедшего времени (по умолчанию в миллисекундах)
    template<typename DurationT = std::chrono::milliseconds>
    int64_t elapsed() const {
        return std::chrono::duration_cast<DurationT>(clock::now() - time).count();
    }

    void reset() {time = clock::now();}

    // В качестве начального времени берется текущее время
    clock::time_point time = {clock::now()};
};
