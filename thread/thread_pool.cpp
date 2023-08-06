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

#include "thread_pool.h"
#include "break_point.h"

namespace trd {

ThreadPool::~ThreadPool()
{
    if (!_stopped) stop();
}

void ThreadPool::start()
{
    _stopped = false;
}

void ThreadPool::stop()
{
    _stopped = true;
    for (Item* it : _threads)
    {
        lock_guard<mutex> locker {it->_sleepMutex}; (void) locker;
        if (it->_sleeps)
            it->_sleepCond.notify_all();
    }
    for (Item* item : _threads)
        while (!item->_finished)
        {
            static chrono::milliseconds timeout {5};
            this_thread::sleep_for(timeout);
        }

    for (Item* item : _threads)
        delete item;
    _threads.clear();
}

void ThreadPool::setTimeout(int timeout)
{
    if (_stopped)
        _timeout = timeout;
}

ThreadPool::Item* ThreadPool::run(Func runFunc)
{
    if (!runFunc)
        return 0;

    // Если пул остановлен, то выполняем функции в исходном потоке
    if (_stopped)
    {
        runFunc();
        return 0;
    }

    lock_guard<mutex> locker {_threadsLock}; (void) locker;

    Item* item = 0;
    for (Item* it : _threads)
    {
        if (it->_working)
            continue;

        if (it->_finished)
        {
            item = it;
            break;
        }

        { //Block for lock_guard
            lock_guard<mutex> locker {it->_sleepMutex}; (void) locker;
            if (it->_sleeps)
            {
                it->_func = std::move(runFunc);
                it->_working = true;
                it->_sleepCond.notify_all();
                return it;
            }
        }
    }

    if (item == 0)
    {
        item = new Item;
        _threads.push_back(item);
    }
    item->_pool = this;
    item->_timeout = chrono::seconds(_timeout);
    item->_working = true;
    item->_sleeps = false;
    item->_finished = false;
    item->_func = std::move(runFunc);
    item->_thread = thread(&Item::run, item);
    item->_thread.detach();
    return item;
}

void ThreadPool::Item::run()
{
    while (true)
    {
        if (_pool->_stopped)
            break;

        if (_func)
        {
            _func();
            _func = nullptr_t(); // Обнуляем рабочую функцию
        }

        // Тонкость: если вызывать _workCond.notify_all()  без блокирования
        // мьютекса _workMutex, то в функции ThreadPool::Item::join() будут
        // периодически  происходить не срабатывания _workCond.wait(),  что
        // фактически будет приводить к блокировке потока,  который  сделал
        // вызов ThreadPool::Item::join()
        {
            lock_guard<mutex> locker {_workMutex}; (void) locker;
            _working = false;
            _workCond.notify_all();
        }
        //asm volatile ("" ::: "memory");

        if (_pool->_stopped)
            break;

        { //Block for unique_lock
            unique_lock<mutex> locker {_sleepMutex};
            _sleeps = true;
            cv_status stat;
            do {
                if (_working || _pool->_stopped)
                    break;

                //static chrono::seconds timeout {15};
                stat = _sleepCond.wait_for(locker, _timeout);

            } while (stat != cv_status::timeout);
            _sleeps = false;
        }
        if (!_working || _pool->_stopped)
            break;
    }
    _finished = true;
}

void ThreadPool::Item::join()
{
    if (_working)
    {
        unique_lock<mutex> locker {_workMutex}; (void) locker;
//        static chrono::microseconds timeout {5};
//        while (!_joinCond.wait_for(locker, timeout, [this]{
//            //return !this->_working.load(memory_order_acquire);
//            return !this->_working;
//        })) ;
        _workCond.wait(locker, [this]{return !this->_working;});
    }
}

ThreadPool& threadPool()
{
    return safe::singleton<ThreadPool>();
}

} // namespace trd
