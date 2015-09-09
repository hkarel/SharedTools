#include "thread_info.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

namespace trd
{

pid_t gettid()
{
#if defined(__FreeBSD__)
    long tid;
    syscall(SYS_thr_self, &tid);
    return pid_t(tid);
#else
    return pid_t(syscall(SYS_gettid));
#endif
}

bool thread_exists(pid_t tid)
{
#if defined(__FreeBSD__)
    long tid_ = long(tid);
    int res = syscall(SYS_thr_kill, tid_, 0);
    return (res != ESRCH)
#else
    int res = syscall(SYS_tkill, tid, 0);
    return ((res == -1) && (errno == ESRCH)) ? false : true;
#endif
}

} // namespace trd

