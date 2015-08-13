#pragma once

#include <mutex>
#include <memory>

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
T& safe_singleton()
{
    static std::unique_ptr<T> t;
    static std::mutex lock;
    if (!t)
    {
        std::lock_guard<std::mutex> locker(lock); (void) locker;
        if (!t)
            t = std::unique_ptr<T>(new T());
    }
    return *t;
}

