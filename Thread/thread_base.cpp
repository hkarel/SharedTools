#include "break_point.h"
#include "thread_base.h"


namespace trd
{
using namespace std;

ThreadBase::ThreadBase()
{}

ThreadBase::~ThreadBase()
{}

void ThreadBase::start()
{
    lock_guard<mutex> locker(_startStopLock); (void) locker;

    //break_point

    if (_thread.joinable())
        return;

    _waitThreadStart = true;
    _threadStop = false;

    _thread = thread(&ThreadBase::runImpl, this);
}

void ThreadBase::stop()
{
    lock_guard<mutex> locker(_startStopLock); (void) locker;

    // Ждем пока поток начнет выполняться, это нужно чтобы не проскочить
    // мимо вызова _thread.join(), см. ниже.
    while (_waitThreadStart) {}

    _threadStop = true;

    // Проверка на _thread.joinable() должна быть обязательно. Это нужно для
    // случаев, когда stop() вызвали раньше чем start() - без проверки получим
    // SIGABRT.
    if (_thread.joinable())
        _thread.join();
}

void ThreadBase::runImpl()
{
    _waitThreadStart = false;
    run();
}

//int ThreadBase::priority()
//{
//    sched_param sch;
//    int policy;
//    pthread_getschedparam(_thread.native_handle(), &policy, &sch);
//    return sch.sched_priority;
//}

//int ThreadBase::setPriority(int val)
//{
//    sched_param sch;
//    int policy;
//    pthread_getschedparam(_thread.native_handle(), &policy, &sch);
//    //sch.sched_priority = val;
//    //sch.sched_priority = 1;
//    //return pthread_setschedparam(_thread.native_handle(), SCHED_FIFO, &sch);
//    return pthread_setschedparam(_thread.native_handle(), SCHED_BATCH, &sch);
//    //return pthread_setschedparam(_thread.native_handle(), SCHED_OTHER, &sch);

//    //return pthread_setschedprio(_thread.native_handle(), 100);
//}


} // namespace trd
