/* clang-format off */
/*****************************************************************************
  В модуле реализован простейший механизм для отсчета времени

*****************************************************************************/

#pragma once

#include <ctime>
#include <chrono>

struct simple_timer
{
    typedef std::chrono::high_resolution_clock clock;

    simple_timer() = default;
    simple_timer(const simple_timer&) = default;
    simple_timer& operator= (const simple_timer&) = default;

    // Задает начальное время
    simple_timer(time_t seconds) : time(clock::from_time_t(seconds)) {}

    // Возвращает количество прошедшего времени (в миллисекундах)
    int elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>
               (clock::now() - time).count();
    }

    // Сбрасывает время на указанное, по умолчанию сбрасывает на текущее время
    void reset(time_t seconds = 0) {
        time = (seconds == 0) ? clock::now() : clock::from_time_t(seconds);
    }

    // В качестве начального времени берется текущее время
    clock::time_point time = {clock::now()};
};
