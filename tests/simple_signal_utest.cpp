/* clang-format off */

// g++ -std=c++11 -ggdb3 -Wall -Wno-unused-but-set-variable simple_signal_utest.cpp -o simple_signal_utest

//#include <unistd.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <assert.h>

#include "../simple_signal.h"


std::mutex mut;
int index5 = 0;

struct Test
{
    std::string* s;

    Test()
    {
        s = new std::string();
    }
    ~Test()
    {
        delete s;
        s = nullptr;
    }


    float F0() {return 3;}
    float FC0() const {return 33;}

    float F(float x) {return x + 1;}
    float FC(float x) const {return x * 2;}

    float F2(float x, float y) {return x + y;}
    float FC2(float x, float y) const {return x * y;}

    float XXX(Closure<float(float, float)> c, double x, int y) const {return c(x, y) + 300;}

    float on_slot(float x, float y)
    {
        //nanosleep(100);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::lock_guard<std::mutex> locker {mut}; (void) locker;

        std::cout << "on_slot: " << x + y  << "   " << ++index5 << *s << '\n';
        //std::cout << x + y  << '\n';
        return 0;
    }

    float on_slot2(float x, float y) const
    {
        std::cout << "on_slot2: " << x + y + 3 << '\n';
        //std::cout << x + y + 5 << '\n';
        return 0;
    }

};


float F5(float x, float y)
{
    std::cout << "F5: " << x + y + 5 << '\n';
    return 0;
}

float F6(const float& x, float* y)
{
    std::cout << "F6: " << x + *y + 6 << '\n';
    return 0;
}


std::atomic_int threads_count {4};
std::atomic_int threads_loop  {0};

typedef Closure<float (float, float)> ThreadFunc;
SimpleSignal<ThreadFunc> thread_signal;

void thread_func(int)
{
    while (threads_loop < 4) {
        std::vector<Test*> tests;
        for (std::size_t i = 0; i < 100; ++i) {
            Test* t = new Test();
            ThreadFunc f = CLOSURE(&Test::on_slot, t);
            thread_signal.connect(f);
            tests.push_back(t);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(400));

        for (std::size_t i = 0; i < tests.size(); ++i) {
            Test* t = tests[i];
            ThreadFunc f = CLOSURE(&Test::on_slot, t);
            thread_signal.disconnect(f);
            delete t;
            //memset(t, 0, sizeof(Test));
        }
    }
    --threads_count;
}


int main()
{
    Test test;

    typedef Closure<float (float, float)> Func;
    Func f  = CLOSURE(&Test::on_slot,  &test);
    Func f2 = CLOSURE(&Test::on_slot2, &test);
    Func f5 = CLOSURE(&F5);

    typedef Closure<float (const float&, float*)> Func6;
    Func6 f6 = CLOSURE(&F6);

    Func f5_disconnect = CLOSURE(&F5);

    // Can not determine the placement of the function empty()
    f.empty();

    std::cout << "signal:\n";
    SimpleSignal<Func> signal;
    SimpleSignal<Func6> signal6;

    // Can not determine the placement of the function test()
    signal.test();

    // Can not determine the placement of the function connect()
    signal.connect(f);
    signal.connect(f2);
    signal.connect(f5);

    assert(signal.exists(f5));
    assert(!signal6.exists(f6));

    signal.emit_(3, 5);

    std::cout << "signal2 (unique connect f5):\n";
    SimpleSignal<float (float, float) > signal2;
    signal2.connect(f);
    signal2.connect(f2);
    signal2.connect(f5);
    signal2.connect(f5);
    signal2.connect(f5);

    signal2.emit_(3, 5);

    std::cout << "signal2 (multi connect f5):\n";
    signal2.connect(f5, false);
    signal2.connect(f5, false);

    signal2.emit_(3, 5);

    // test equivalent operators
    assert(f2 != f5);
    assert(f5 == f5);

    std::cout << "signal2 (disconnect one f5):\n";
    signal2.disconnect(f5_disconnect, false);
    signal2.emit_(3, 5);

    std::cout << "signal2 (disconnect all f5):\n";
    signal2.disconnect(f5_disconnect);
    signal2.emit_(3, 5);

    std::cout << "signal6 (emit):\n";
    signal6.connect(f6);
    float y6 = 20;
    signal6.emit_(3, &y6);


    //SimpleSignal<Closure<float (float,float)> > signal(f);
    //SimpleSignal<Func> signal;

    std::cout << "connect signal2.emit() to signal:\n";
    Func s = CLOSURE(&SimpleSignal<Func>::emit_, &signal2);
    signal.connect(s);
    signal.emit_(3, 5);

    //return 0;

    SimpleSignal<ThreadFunc>& th_signal = thread_signal; (void) th_signal;

    // Тест создания/вызова/удаления слотов из нескольких потоков
    std::thread t1(thread_func, 0);
    std::thread t2(thread_func, 0);
    std::thread t3(thread_func, 0);
    std::thread t4(thread_func, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    while (threads_count) {

        thread_signal.emit_(3, threads_loop);
        //std::this_thread::sleep_for(std::chrono::milliseconds(200));
        //std::this_thread::sleep_for(std::chrono::seconds(2));

        if (threads_loop == 1) {
            thread_signal.clear();
            std::cout << "thread_signal.clear() \n";

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "milliseconds(500) \n";

            thread_signal.emit_(3, 10);
        }

        ++threads_loop;
    }
    //if (threads_count == 0)
    //    break_point

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    std::cout << "Multithread test PASSED\n";

    return 0;
}
