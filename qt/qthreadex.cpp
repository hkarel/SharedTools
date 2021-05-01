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

#include "qthreadex.h"
#include "defmac.h"
#include "break_point.h"
#include <QElapsedTimer>

QThreadEx::QThreadEx() : QThreadEx(nullptr)
{}

QThreadEx::QThreadEx(QObject* parent) : QThread(parent)
{
    chk_connect_d(this, &QThread::started,  this, &QThreadEx::onStarted)
    chk_connect_d(this, &QThread::finished, this, &QThreadEx::onFinished)
}

bool QThreadEx::threadStop() const noexcept
{
    return _threadStop;
}

void QThreadEx::start(Priority priority)
{
    startImpl(priority);
}

bool QThreadEx::stop(unsigned long timeout)
{
    return stopImpl(timeout);
}

void QThreadEx::startImpl(Priority priority)
{
    QMutexLocker locker {&_startStopLock}; (void) locker;

    //while (_waitThreadStart)
    //    usleep(100);

    if (isRunning())
        return;

    _threadStop = false;
    _waitThreadStart = true;

    QThread::start(priority);

    // Замечание для Qt 4.8: статус isRunning выставляется в TRUE в самом начале
    // вызова функции start(). Если на момент выхода из start() статус isRunning
    // равен FALSE - значит при запуске потока что-то пошло не так и статус был
    // аннулирован
    if (!isRunning())
        _waitThreadStart = false;

    // Ждем пока поток запустится
    while (_waitThreadStart)
    {
        usleep(100);
        if (isFinished())
            _waitThreadStart = false;
    }
}

bool QThreadEx::stopImpl(unsigned long timeout)
{
    QMutexLocker locker {&_startStopLock}; (void) locker;

    while (_waitThreadStart)
        usleep(100);

    _threadStop = true;
    threadStopEstablished();

    bool res = true;
    if (isRunning() && timeout)
        res = wait(timeout);
    return res;
}

void QThreadEx::sleep(unsigned long timeout) const
{
    timeout *= 1000;
    QElapsedTimer timer;
    timer.start();
    while (!timer.hasExpired(timeout) && !threadStop())
        QThread::msleep(200);
}

void QThreadEx::onStarted()
{
    _waitThreadStart = false;
}

void QThreadEx::onFinished()
{
    _threadStop = true;
}
