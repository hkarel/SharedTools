/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2023 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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
  ---

  Простой механизм для измерения времени выполнения циклических операций.

  Декларация:
    DECL_PROCESSING_TIME_BEGIN(100) // Количество итераций
    DECL_PROCESSING_TIME_LABEL(10)  // Метки для вывода в лог 10, 20, 30
    DECL_PROCESSING_TIME_LABEL(20)
    DECL_PROCESSING_TIME_LABEL(30)
    DECL_PROCESSING_TIME_END

  Рабочий код:
    while (true) {
      RESET_PROCESSING_TIME
      ...
      LOG_PROCESSING_TIME(10)
      ...
      LOG_PROCESSING_TIME(20)
      ...
      LOG_PROCESSING_TIME(30)
      INDEXUP_PROCESSING_TIME
    }

*****************************************************************************/

#pragma once

#include "shared/utils.h"
#include "shared/steady_timer.h"
#include "shared/logger/logger.h"
#include <vector>
#include <stdint.h>

#define DECL_PROCESSING_TIME_BEGIN(COUNT) \
    struct { \
        bool enabled = {true}; \
        size_t index = {0}; \
        const size_t count = {COUNT}; \
        steady_timer timer; \
        struct time_array : std::vector<int64_t> \
        { \
            time_array(size_t num) {reserve(num);} \
        };

#define DECL_PROCESSING_TIME_LABEL(LABEL) \
        time_array test##LABEL {count};

#define DECL_PROCESSING_TIME_END \
    } __processing_time__;

#define ENABLED_PROCESSING_TIME(VAL) \
    __processing_time__.enabled = VAL;

#define RESET_PROCESSING_TIME \
    if (__processing_time__.enabled && (alog::logger().level() == alog::Level::Debug2)) { \
        __processing_time__.timer.reset(); \
    }

#define TRACE_PROCESSING_TIME(LABEL) \
    if (__processing_time__.enabled && (alog::logger().level() == alog::Level::Debug2)) { \
        __processing_time__.test##LABEL.push_back(__processing_time__.timer.elapsed<chrono::microseconds>()); \
    }

#define PRINT_PROCESSING_TIME(LABEL) \
    if (__processing_time__.enabled && (alog::logger().level() == alog::Level::Debug2)) { \
        if (__processing_time__.index == (__processing_time__.count - 1)) { \
            log_debug2_m << "Processing time (average) "#LABEL << ": " \
                         << alog::round(utl::average(__processing_time__.test##LABEL), 3) << " us"; \
            __processing_time__.test##LABEL.clear(); \
    }}

/**
  LOG_PROCESSING_TIME объединяет два макроса: TRACE_PROCESSING_TIME и PRINT_PROCESSING_TIME
*/
#define LOG_PROCESSING_TIME(LABEL) \
    if (__processing_time__.enabled && (alog::logger().level() == alog::Level::Debug2)) { \
        /* TRACE_PROCESSING_TIME */ \
        __processing_time__.test##LABEL.push_back(__processing_time__.timer.elapsed<chrono::microseconds>()); \
        /* PRINT_PROCESSING_TIME */ \
        if (__processing_time__.index == (__processing_time__.count - 1)) { \
            log_debug2_m << "Processing time (average) "#LABEL << ": " \
                         << alog::round(utl::average(__processing_time__.test##LABEL), 3) << " us"; \
            __processing_time__.test##LABEL.clear(); \
    }}

#define INDEXUP_PROCESSING_TIME \
    if (__processing_time__.enabled && (alog::logger().level() == alog::Level::Debug2)) { \
        if (++__processing_time__.index == __processing_time__.count) \
            __processing_time__.index = 0; \
    }
