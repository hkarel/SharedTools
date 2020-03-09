/*****************************************************************************
  The MIT License

  Copyright © 2020 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  Модуль содержит специализированные потоковые операторы '<<' для логгера.

*****************************************************************************/

#pragma once

#include "utils.h"
#include "logger/logger.h"
#include "qt/quuidex.h"

#include <string>
#include <tuple>
#include <vector>

namespace alog {

using namespace std;

/**
  Класс реализует механизм форматированного вывода сообщений.
  Пример использования:
    log_debug << log_format("Param1: ?. Param2 [?]. Param3 ?", 1, 2, 3);
*/
template<typename... Args> class Format
{
public:
    Format(const string& descript, Args&&... args)
        : _args(std::forward<Args>(args)...)
    {
        _chunks = utl::split(descript, '?', true);
    }

    void init(Line* line)
    {
        _line = line;
    }
    void build()
    {
        callBuildFunc(_args, index_sequence_for<Args...>());
    }

private:
    // Решение описано тут:
    // https://www.murrayc.com/permalink/2015/12/05/modern-c-variadic-template-parameters-and-tuples/
    template<std::size_t... Is>
    void callBuildFunc(const tuple<Args...>& tuple, index_sequence<Is...>)
    {
        if (!_line)
            return;

        buildFunc(std::get<Is>(tuple)...);
    }

    template<typename T, typename... Ts>
    void buildFunc(T&& t, Ts&&... ts)
    {
        bool chunkPrint = false;
        if (!_chunks.empty())
        {
            chunkPrint = true;
            *_line << _chunks[0];
            _chunks.erase(_chunks.begin());
        }

        if (!chunkPrint)
            *_line << ",";

        *_line << t;
        buildFunc(ts...);
    }
    void buildFunc()
    {
        for (size_t i = 0; i < _chunks.size(); ++i)
        {
            *_line << _chunks[i];
            if (i != (_chunks.size() - 1))
                *_line << "?";
        }
    }

private:
    Line* _line = {0};
    vector<string> _chunks;
    tuple<Args...> _args;

    template<typename... A>
    friend Line& operator<< (Line&, const Format<A...>&);
};

template<typename... Args>
inline Format<Args...> format(const string& descript,  Args&&... args)
{
    return Format<Args...>(descript, std::forward<Args>(args)...);
}

template<typename... Args>
inline Format<Args...> format(const char* descript, Args&&... args)
{
    return format(string(descript), std::forward<Args>(args)...);
}

template<typename... Args>
Line& operator<< (Line& line, const Format<Args...>& fmt)
{
    if (line.toLogger())
    {
        const_cast<Format<Args...>&>(fmt).init(&line);
        const_cast<Format<Args...>&>(fmt).build();
    }
    return line;
}

} // namespace alog

#define log_format alog::format
