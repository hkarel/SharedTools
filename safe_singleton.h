#pragma once

#include "simple_ptr.h"
#include "spin_locker.h"

// Безопасно создает singleton
template<typename T, int = 0>
T& safe_singleton()
{
    static simple_ptr<T> t;
    static std::atomic_flag lock{ATOMIC_FLAG_INIT};
    if (!t)
    {
        SpinLocker locker(lock); (void) locker;
        if (!t)
            t = simple_ptr<T>(new T());
    }
    return *t;
}

