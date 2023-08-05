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

#include <mutex>
#include <memory>

namespace safe {

/**
  Функция реализует механизм создания потокобезопасного singleton. Механизм
  основан на паттерне Double-Checked Locking Pattern (DCLP) описанном у
  Scott Meyers и Andrei Alexandrescu.
  Второй шаблонный параметр позволяет делать исключения из концепции singleton
  и создавать несколько экземпляров одного и того же класса.

  Замечание: По мнению авторов идеи механизм DCLP может некорректно работать
             на многопроцессорных системах. Поэтому целесообразно делать фиктивный
             вызов safe_singleton() в основном потоке при старте программы.

             [Karelin] 13.08.15
             Работа функции проверена на многопроцессорной системе (архитектура Intel).
             Выполнялся тест safe_singleton_utest.cpp на реальном и на виртуальном
             двухпроцессорных стендах. Сборка теста выполнялась с ключами компиляции
             -O2 и -O3 (g++ 4.7). Проблем в работе функции выявлено не было.
*/
template<typename T, int = 0>
T& singleton()
{
    static std::unique_ptr<T> t;
    static std::mutex lock;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
    if (!t)
#else
    if (__builtin_expect(!t, 0))
#endif
    {
        std::lock_guard<std::mutex> locker {lock}; (void) locker;
        if (!t)
            t = std::unique_ptr<T>(new T());
    }
    return *t;
}

/**
  Экспериментальная функция.
  Возвращает smart-ptr экземпляр
*/
template<typename T, int = 0>
typename T::Ptr& singleton_ptr()
{
    static typename T::Ptr t;
    static std::mutex lock;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
    if (!t)
#else
    if (__builtin_expect(!t, 0))
#endif
    {
        std::lock_guard<std::mutex> locker {lock}; (void) locker;
        if (!t)
            t = typename T::Ptr(new T());
    }
    return t;
}

} // namespace safe
