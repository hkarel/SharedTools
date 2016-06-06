/* clang-format off */
#include "thread_info.h"
#include "break_point.h"

#ifdef __MINGW32__
#include <processthreadsapi.h>
#else
#include <sys/syscall.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <signal.h>

namespace trd {

pid_t gettid()
{
#if defined(__FreeBSD__)
    long tid;
    syscall(SYS_thr_self, &tid);
    return pid_t(tid);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    return pid_t(GetCurrentThreadId());
#else
    return pid_t(syscall(SYS_gettid));
#endif
}

bool thread_exists(pid_t tid)
{
#if defined(__FreeBSD__)
    long tid_ = long(tid);
    int res = syscall(SYS_thr_kill, tid_, 0);
    return (res != ESRCH);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    // Отладить проверку существования потока через функцию OpenThread()
    break_point
    DWORD tid_ = DWORD(tid);
    HANDLE h = OpenThread(THREAD_ALL_ACCESS, FALSE, tid_);
    // ???  GetLastError()
    return (h != 0);
#else
    int res = syscall(SYS_tkill, tid, 0);
    return ((res == -1) && (errno == ESRCH)) ? false : true;
#endif
}

} // namespace trd
