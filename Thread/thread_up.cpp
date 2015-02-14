#include <cstdio>
#include <unistd.h>
#include <pthread.h>
#include "thread_up.h"


namespace trd
{
using namespace std;

int ThreadUp::priorityUp(int schedulAlgorithm, int priority)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
    //
    // Повышаем приоритет потока
    // http://man.sourcentral.org/f17/ru/2+sched_setscheduler
    //

    sched_param sch;
    int sched_policy;
    pthread_getschedparam(nativeHandle(), &sched_policy, &sch);
    sch.sched_priority = priority;
    int err = pthread_setschedparam(nativeHandle(), schedulAlgorithm, &sch);
    //int err = pthread_setschedparam(nativeHandle(), SCHED_FIFO, &sch);
    //int err = pthread_setschedparam(nativeHandle(), SCHED_BATCH, &sch);
    //int err = pthread_setschedparam(nativeHandle(), SCHED_OTHER, &sch);

    _priorityUpError.clear();

    if (err != 0)
    {
        string s;
        if (err == EPERM)
            s = "Perhaps insufficiently right for a performance of this operation. ";

        char buff[34] = {0};
        snprintf(buff, sizeof(buff)-1, "%d", err);

        _priorityUpError = "Couldn't increase priority of a thread. "
                         + s + "Code error: " + buff;
    }
#endif // _POSIX_PRIORITY_SCHEDULING

    return err;
}

} // namespace trd