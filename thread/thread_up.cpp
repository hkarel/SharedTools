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

#include "thread_up.h"

#ifdef __GNUC__
#include <cstdio>
#include <unistd.h>
#include <pthread.h>
#endif

namespace trd {

int ThreadUp::priorityUp(int schedulAlgorithm, int priority)
{
    int err = 0;

#ifdef _POSIX_PRIORITY_SCHEDULING
    //
    // Повышаем приоритет потока
    // http://man.sourcentral.org/f17/ru/2+sched_setscheduler
    //

    sched_param sch;
    int sched_policy;
    pthread_getschedparam(nativeHandle(), &sched_policy, &sch);
    sch.sched_priority = priority;
    err = pthread_setschedparam(nativeHandle(), schedulAlgorithm, &sch);
    //int err = pthread_setschedparam(nativeHandle(), SCHED_FIFO, &sch);
    //int err = pthread_setschedparam(nativeHandle(), SCHED_BATCH, &sch);
    //int err = pthread_setschedparam(nativeHandle(), SCHED_OTHER, &sch);

    _priorityUpError.clear();

    if (err != 0)
    {
        std::string s;
        if (err == EPERM)
            s = "Perhaps insufficiently right for a performance of this operation.";

        char buff[34] = {0};
        snprintf(buff, sizeof(buff) - 1, "%d", err);

        _priorityUpError = "Couldn't increase priority of a thread. ";
        _priorityUpError += s + " Code error: " + buff;
    }
#endif // _POSIX_PRIORITY_SCHEDULING

    return err;
}

} // namespace trd
