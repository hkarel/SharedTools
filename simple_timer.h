/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2013 Pavel Karelin (hkarel), <hkarel@yandex.ru>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*****************************************************************************/

#pragma once

#include <ctime>
#include <chrono>

/**
  Простейший механизм для отсчета интервалов времени
*/
struct simple_timer
{
    typedef std::chrono::system_clock clock;

    simple_timer() = default;
    simple_timer(const simple_timer&) = default;
    simple_timer& operator= (const simple_timer&) = default;

    // Задает начальное время
    simple_timer(time_t seconds) : time(clock::from_time_t(seconds)) {}

    // Возвращает количество прошедшего времени (по умолчанию в миллисекундах)
    template<typename DurationT = std::chrono::milliseconds>
    int64_t elapsed() const {
        return std::chrono::duration_cast<DurationT>(clock::now() - time).count();
    }

    // Сбрасывает время на указанное, по умолчанию сбрасывает на текущее время
    void reset(time_t seconds = 0) {
        time = (seconds == 0) ? clock::now() : clock::from_time_t(seconds);
    }

    // В качестве начального времени берется текущее время
    clock::time_point time = {clock::now()};
};
