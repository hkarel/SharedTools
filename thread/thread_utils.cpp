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

#include "thread_utils.h"
#include "break_point.h"
#include "spin_locker.h"

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#include <processthreadsapi.h>
#else
#include <sys/syscall.h>
#endif
#include <cerrno>
#include <csignal>
#include <unistd.h>

namespace trd {

bool ThreadIdList::empty() const
{
    SpinLocker locker {_tidsLock}; (void) locker;
    return _tids.empty();
}

void ThreadIdList::lock(std::function<void (const std::vector<pid_t>&)> func)
{
    SpinLocker locker {_tidsLock}; (void) locker;
    func(_tids);
}

ThreadIdLock::ThreadIdLock(ThreadIdList* l) : _threadIdList(l)
{
    SpinLocker locker {_threadIdList->_tidsLock}; (void) locker;
    _threadIdList->_tids.push_back(trd::gettid());
}

ThreadIdLock::~ThreadIdLock()
{
    SpinLocker locker {_threadIdList->_tidsLock}; (void) locker;
    pid_t tid = trd::gettid();
    auto it = _threadIdList->_tids.begin();
    for (; it != _threadIdList->_tids.end(); ++it)
        if (*it == tid)
            _threadIdList->_tids.erase(it--);
}

pid_t gettid()
{
#if defined(__FreeBSD__)
    long tid;
    syscall(SYS_thr_self, &tid);
    return pid_t(tid);
#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
    return pid_t(GetCurrentThreadId());
#else
    return pid_t(syscall(SYS_gettid));
#endif
}

bool threadExists(pid_t tid)
{
#if defined(__FreeBSD__)
    long tid_ = long(tid);
    long res = syscall(SYS_thr_kill, tid_, 0);
    return (res != ESRCH);
#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
    // Отладить проверку существования потока через функцию OpenThread()
    break_point
    DWORD tid_ = DWORD(tid);
    HANDLE h = OpenThread(THREAD_ALL_ACCESS, FALSE, tid_);
    // ???  GetLastError()
    return (h != 0);
#else
    long res = syscall(SYS_tkill, tid, 0);
    return ((res == -1) && (errno == ESRCH)) ? false : true;
#endif
}

} // namespace trd
