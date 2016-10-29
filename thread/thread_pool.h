/* clang-format off */
#pragma once

#include "defmac.h"
#include "spin_locker.h"
#include "safe_singleton.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include <condition_variable>

namespace trd {

using namespace std;

/**
  Простой пул потоков
*/
class ThreadPool
{
public:
    typedef function<void()> Func;

    ThreadPool() = default;
    ~ThreadPool();

    class Item
    {
    public:
        Item() = default;

        // Применяется для ожидания завершения процесса вызова рабочей функции.
        // Пример использования:
        //   void func(int a, int b) {a + b;}
        //   int a = 1, b = 2;
        //   vector<ThreadPool::Item*> threads;
        //   for (int i = 0; i < 5; ++i)
        //       if (ThreadPool::Item* thread = threadPool().run(func, a, b))
        //           threads.push_back(thread);
        //   for (ThreadPool::Item* thread : threads)
        //       thread->join();
        //
        void join();

    private:
        DISABLE_DEFAULT_COPY(Item)

        void run();

        Func _func;
        thread _thread;
        ThreadPool* _pool;

        atomic_bool   _working  = {true};  // Признак, что поток выполняет работу.
        volatile bool _sleeps   = {false}; // Поток находится в состоянии ожидания.
        volatile bool _finished = {false}; // Поток уже закончил работу.

        mutex _workMutex;
        condition_variable _workCond;

        mutex _sleepMutex;
        condition_variable _sleepCond;

        friend class ThreadPool;
    };

    // Активирует работу пула потоков.
    void start();

    // Останавливает работу пула потоков. Функция должна вызываться перед
    // завершением работы программы.
    void stop();

    // Возвращает TRUE, когда пул потоков остановлен.
    bool stopped() const {return _stopped;}

    // Выполняет функцию runFunc в отдельном потоке, при условии, что пул
    // потоков не остановлен. Если пул потоков остановлен, то функция runFunc
    // будет выполнена в вызывающем потоке.
    ThreadPool::Item* run(Func runFunc);

    // Используется для выполнения функции func c произвольной сигнатурой.
    template<typename FuncType, typename... Args>
    ThreadPool::Item* run(FuncType func, Args... args)
    {
        if (_stopped) return 0;
        Func runFunc = std::bind(func, std::forward<Args>(args)...);
        return run(runFunc);
    }

private:
    DISABLE_DEFAULT_COPY(ThreadPool)

    atomic_bool _stopped = {false};
    vector<Item*> _threads;
    mutex _threadsLock;

    template<typename T, int> friend T& ::safe_singleton();
};
ThreadPool& threadPool();

} // namespace trd
