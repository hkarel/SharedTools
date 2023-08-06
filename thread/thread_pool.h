/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2015 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "defmac.h"
#include "spin_locker.h"
#include "safe_singleton.h"

#include <atomic>
#include <chrono>
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
        chrono::seconds _timeout;

        atomic_bool   _working  = {true};  // Признак, что поток выполняет работу
        volatile bool _sleeps   = {false}; // Поток находится в состоянии ожидания
        volatile bool _finished = {false}; // Поток уже закончил работу

        mutex _workMutex;
        condition_variable _workCond;

        mutex _sleepMutex;
        condition_variable _sleepCond;

        friend class ThreadPool;
    };

    // Активирует работу пула потоков
    void start();

    // Останавливает работу пула потоков. Функция должна вызываться перед завер-
    // шением работы программы
    void stop();

    // Возвращает TRUE, когда пул потоков остановлен
    bool stopped() const {return _stopped;}

    // Интервал времени (в секундах) по истечении которого неиспользуемый поток
    // будет уничтожен. Установить новый интервал можно только когда пул потоков
    // остановлен. По умолчанию интервал равен 15
    int timeout() const {return _timeout;}
    void setTimeout(int timeout);

    // Выполняет функцию runFunc в отдельном потоке, при условии, что пул пото-
    // ков не остановлен. Если пул потоков остановлен, то функция runFunc будет
    // выполнена в вызывающем потоке
    ThreadPool::Item* run(Func runFunc);

    // Используется для выполнения функции func c произвольной сигнатурой
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
    atomic_int  _timeout = {15};

    vector<Item*> _threads;
    mutex _threadsLock;

    template<typename T, int> friend T& safe::singleton();
};

ThreadPool& threadPool();

} // namespace trd
