#include "break_point.h"
#include "thread_base.h"


namespace trd
{
using namespace std;

ThreadBase::ThreadBase()
{
    _threadStop = true;
    _waitThreadStart = false;
}

ThreadBase::~ThreadBase()
{}

void ThreadBase::start()
{
    startImpl();
}

void ThreadBase::startImpl()
{
    lock_guard<mutex> locker(_startStopLock); (void) locker;

    //break_point

    // Для ситуаций когда метод start() будет вызван несколько раз подряд,
    // при этом метод _thread.joinable() не сразу вернет TRUE, и мы получим
    // перезатирание переменной _thread при повторных вызовах метода start().
    // Чтобы этого избежать ждем пока поток начнет работать и условие
    // _thread.joinable() == TRUE будет выполнятся.
    while (_waitThreadStart) {}

    if (_thread.joinable())
        return;

    _waitThreadStart = true;
    _threadStop = false;

    _thread = thread(&ThreadBase::runHandler, this);
}

void ThreadBase::stop(bool wait)
{
    stopImpl(wait);
}

void ThreadBase::stopImpl(bool wait)
{
    lock_guard<mutex> locker(_startStopLock); (void) locker;

    // Ждем пока поток начнет выполняться, это нужно чтобы не проскочить
    // мимо вызова _thread.join(), см. ниже.
    while (_waitThreadStart) {}

    _threadStop = true;

    // Проверка на _thread.joinable() должна быть обязательно. Это нужно для
    // случаев, когда stop() вызвали раньше чем start() - без проверки получим
    // SIGABRT.
    if (wait && _thread.joinable())
        _thread.join();
}

void ThreadBase::runHandler()
{
    _waitThreadStart = false;
    run();
    _threadStop = true;
}

} // namespace trd
