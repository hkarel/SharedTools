#pragma once

#include <mutex>
#include <memory>

// Безопасно создает singleton. Второй шаблонный параметр дает возможность
// создавать несколько экземпляров singleton-ов для одного и того же типа T.
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

