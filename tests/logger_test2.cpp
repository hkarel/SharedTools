/* clang-format off */

#include "steady_timer.h"
#include "logger/logger.h"
#include <signal.h>

#include <stdio.h>
#include <atomic>
#include <thread>
#include <vector>

#define log_error_m   alog::logger().error  (__FILE__, __func__, __LINE__, "MainModule")
#define log_warn_m    alog::logger().warn   (__FILE__, __func__, __LINE__, "MainModule")
#define log_info_m    alog::logger().info   (__FILE__, __func__, __LINE__, "MainModule")
#define log_verbose_m alog::logger().verbose(__FILE__, __func__, __LINE__, "MainModule")
#define log_debug_m   alog::logger().debug  (__FILE__, __func__, __LINE__, "MainModule")
#define log_debug2_m  alog::logger().debug2 (__FILE__, __func__, __LINE__, "MainModule")

using namespace std;
using namespace std::chrono;

//Use this macro to switch on multi-threading mode
#define MULTI_THREAD

int main(int argc, char* argv[])
{
    alog::logger().start();

    //Logger initialization

    // Создаем дефолтный сэйвер для логгера
    {
        //std::string logLevelStr = "debug2";
        //config::base().getValue("logger.level", logLevelStr);

        bool logContinue = false;
        //config::base().getValue("logger.continue", logContinue);

        alog::Level logLevel = alog::Level::Debug2;
        alog::SaverPtr saver {new alog::SaverFile("default",
                                                  "/tmp/logget_test2.log",
                                                  logLevel,
                                                  logContinue)};
        alog::logger().addSaver(saver);
    }

    log_info << "LoggerTest2 is running";
    alog::logger().flush();

    unsigned int thread_count = 4; (void) thread_count;
    unsigned int howmany = 1*1000*1000;

    vector<thread> threads; (void) threads;
    auto start = system_clock::now();

#if !defined(MULTI_THREAD)
    for (unsigned int i = 0; i < howmany; ++i)
    {
        //Has to be customized for every logger
        log_info_m << "Message + all required information, #" << i;
    }
#else
    howmany /= thread_count;
    for (unsigned int t = 0; t < thread_count; ++t)
    {
        threads.push_back(std::thread([&]
        {
            for (unsigned int i = 0; i < howmany; ++i)
            {
                //Has to be customized for every logger
                log_info_m << "Message + all required information, #" << i;
            }
        }));
    }

    for (auto &t:threads)
    {
        t.join();
    };

    howmany *= thread_count;

#endif

    alog::logger().flush();
    alog::logger().waitingFlush();

    auto delta = system_clock::now() - start;
    auto delta_d = duration_cast<duration<double>> (delta).count();

    log_info_m << "Time = " << (double)howmany / delta_d << " per second, total time = "  << delta_d;

    //Logger uninitialization if necessary

    log_info << "LoggerTest2 is stopped";
    alog::logger().flush();
    alog::logger().waitingFlush();
    alog::logger().stop();

    return 0;
}

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
