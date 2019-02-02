#include "logger/logger.h"
#include <stdlib.h>
#include <unistd.h>

void prog_abort()
{
    log_error << "Program aborted";
    alog::logger().flush();
    alog::logger().waitingFlush();
    alog::logger().stop();
    usleep(200*1000);
    abort();
}
