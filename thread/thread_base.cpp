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

#include "thread_base.h"
#include "break_point.h"

namespace trd {

//ThreadBase::ThreadBase()
//{
//    _threadRun = false;
//    _threadStop = true;
//    _waitThreadStart = false;
//    //_waitThreadStop = true;
//}

bool ThreadBase::threadStop() const noexcept
{
    return _threadStop;
}

bool ThreadBase::threadRun() const noexcept
{
//    thread::native_handle_type thr = nativeHandle();
//    if (thr != 0)
//    {
//        int res = pthread_kill(thr, 0);
//        return !(ESRCH == res);
//    }
//    return false;

    return _threadRun;
}

std::thread::native_handle_type ThreadBase::nativeHandle() noexcept
{
    return _thread.native_handle();
}

void ThreadBase::start()
{
    startImpl();
}

void ThreadBase::startImpl()
{
    std::lock_guard<std::mutex> locker {_startStopLock}; (void) locker;

    //break_point

    // Для ситуаций когда метод start() будет вызван несколько раз подряд,
    // при этом метод _thread.joinable() не сразу вернет TRUE, и мы получим
    // перезатирание переменной _thread при повторных вызовах метода start().
    // Чтобы этого избежать ждем пока поток начнет работать и условие
    // _thread.joinable() == TRUE будет выполнятся.
    while (_waitThreadStart) {}

    if (threadRun())
        return;

    _waitThreadStart = true;
    _threadStop = false;
    _threadRun = true;

    _thread = std::thread(&ThreadBase::runHandler, this);
}

void ThreadBase::stop(bool wait)
{
    stopImpl(wait);
}

void ThreadBase::stopImpl(bool /*wait*/)
{
    std::lock_guard<std::mutex> locker {_startStopLock}; (void) locker;

    // Примечание: входящий параметр wait на данный момент не используется,
    // так как не удалось добиться стабильной работы системы при асинхронном
    // (т.е. без join()) завершении работы потока.

    // Ждем пока поток начнет выполняться, это нужно чтобы не проскочить
    // мимо вызова _thread.join(), см. ниже.
    while (_waitThreadStart) {}

    //_waitThreadStop = wait;
    _threadStop = true;

    // Проверка на _thread.joinable() должна быть обязательно. Это нужно для
    // случаев, когда stop() вызвали раньше чем start() - без проверки получим
    // SIGABRT.
    if (_thread.joinable())
        _thread.join();
}

void ThreadBase::runHandler()
{
    try
    {
        _waitThreadStart = false;
        //_waitThreadStop = true;

        run();

        // *** Неудачная попытка завершить работу потока асинхронно ***
        //if (!_waitThreadStop && _thread.joinable())
        //    _thread.detach();

        _threadRun = false;
        _threadStop = true;
    }
    catch (...)
    {
        _threadRun = false;
        _threadStop = true;
    }
}

} // namespace trd
