#include "qthreadex.h"
#include "defmac.h"
#include "break_point.h"
#include <QElapsedTimer>

QThreadEx::QThreadEx(QObject * parent) : QThread(parent)
{
    _threadStop = true;
    _waitThreadStart = false;

    chk_connect_d(this, SIGNAL(started()),  this, SLOT(onStarted()))
    chk_connect_d(this, SIGNAL(finished()), this, SLOT(onFinished()))
}

bool QThreadEx::threadStop() const noexcept
{
    return _threadStop;
}

void QThreadEx::start(Priority priority)
{
    startImpl(priority);
}

bool QThreadEx::stop(unsigned long time)
{
    return stopImpl(time);
}

void QThreadEx::startImpl(Priority priority)
{
    QMutexLocker locker(&_startStopLock); (void) locker;

    while (_waitThreadStart) {}

    if (isRunning())
        return;

    _waitThreadStart = true;
    _threadStop = false;

    QThread::start(priority);

    // Ждем пока поток запустится
    while (!isRunning()) {}
}

bool QThreadEx::stopImpl(unsigned long time)
{
    QMutexLocker locker(&_startStopLock); (void) locker;

    while (_waitThreadStart) {}

    _threadStop = true;
    threadStopEstablished();

    bool res = true;
    if (isRunning())
        res = wait(time);
    return res;
}

void QThreadEx::sleep(unsigned long timeout)
{
    timeout *= 1000;
    QElapsedTimer timer;
    timer.start();
    while ((timer.elapsed() < qint64(timeout)) && !threadStop())
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
