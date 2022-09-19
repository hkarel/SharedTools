/*****************************************************************************
  Тест скорости доступа к полю структуры 'A' при использовании shared_ptr
  и container_ptr

*****************************************************************************/

// Команда для сборки
// g++ -std=c++17 -O2 -DNDEBUG shared_ptr_speed.cpp -o shared_ptr_speed

#include "../container_ptr.h"
#include "../steady_timer.h"
#include <memory>
#include <cstdio>

int main()
{
    struct A {int b;};
    steady_timer timer;
    std::shared_ptr<A> a1 {new A};
    container_ptr<A>   a2 {new A};
    int tm1, tm2;

    int sz1 = int(sizeof(std::shared_ptr<A>));
    int sz2 = int(sizeof(container_ptr<A>));

    printf("shared_ptr size %d\n", sz1);
    printf("container_ptr size %d\n", sz2);

    for (int k = 0; k < 10; ++k)
    {
        std::srand(std::time(0));

        timer.reset();
        for (int i = 0; i < 10*1000000; ++i)
        {
            a1->b = std::rand();
        }
        (void) a1->b;
        tm1 = timer.elapsed<std::chrono::microseconds>();

        timer.reset();
        for (int i = 0; i < 10*1000000; ++i)
        {
            a2->b = std::rand();
        }
        (void) a2->b;
        tm2 = timer.elapsed<std::chrono::microseconds>();

        int persent = (1.0 - float(tm1) / tm2) * 100;
        printf("shared_ptr time %d us, container_ptr time %d us, delta: %d%%\n", tm1, tm2, persent);
    }
    return 0;
}
