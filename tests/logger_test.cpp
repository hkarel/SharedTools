/* clang-format off */

#include "steady_timer.h"
#include "logger/logger.h"
#include <signal.h>

#define log_error_m   alog::logger().error_f   (alog_line_location, "TetsModule")
#define log_warn_m    alog::logger().warn_f    (alog_line_location, "TetsModule")
#define log_info_m    alog::logger().info_f    (alog_line_location, "TetsModule")
#define log_verbose_m alog::logger().verbose_f (alog_line_location, "TetsModule")
#define log_debug_m   alog::logger().debug_f   (alog_line_location, "TetsModule")
#define log_debug2_m  alog::logger().debug2_f  (alog_line_location, "TetsModule")

using namespace std;
using namespace alog;

volatile bool stop = false;

void stopProgramHandler(int sig)
{
    if ((sig == SIGTERM) || (sig == SIGINT))
    {
        stop = true;
    }
}

class SaverNull : public Saver
{
public:
    SaverNull(const string& name, Level level = Error) : Saver(name, level)
    {}
    void flushImpl(const MessageList& messages) override
    {
        if (messages.size() == 0)
            return;

        int pause = std::max(5, int(messages.count() * 0.75));
        this_thread::sleep_for(chrono::microseconds(pause));
    }
};
typedef clife_ptr<SaverNull> SaverNullPtr;

void saveToLog()
{
    char mbstr[200] = {0};
    time_t t = time(NULL);
    strftime(mbstr, sizeof(mbstr), "%d.%m.%Y %H:%M:%S ", localtime(&t));
    chrono::milliseconds pause {10000};

    steady_timer elapsedTimer;

    cout << "Begin item test: " << mbstr << endl;
    cout << "Step 1 of 8" << endl;
    cout.flush();

    for (int i = 0; i < 1000000; ++i)
    {
        if (stop)
            break;

        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i << "a1";
        log_debug_m  << "bbbbb o" << i << "a2";
        log_debug2_m << "sssss o" << i << "a3";
    }

    cout << "Step 2 of 8" << endl;
    cout.flush();

    if (!stop)
        this_thread::sleep_for(pause);

    for (int i = 0; i < 1000000; ++i)
    {
        if (stop)
            break;

        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "b1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "b2";
        log_debug2_m << "sssss o" << i + 1000000 << "b3";
    }

    cout << "Step 3 of 8" << endl;
    cout.flush();

    for (int i = 0; i < 1000000; ++i)
    {
        if (stop)
            break;

        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "c1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "c2";
        log_debug2_m << "sssss o" << i + 1000000 << "c3";
    }

    cout << "Step 4 of 8" << endl;
    cout.flush();

    for (int i = 0; i < 1000000; ++i)
    {
        if (stop)
            break;

        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "d1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "d2";
        log_debug2_m << "sssss o" << i + 1000000 << "d3";
    }

    cout << "Step 5 of 8" << endl;
    cout.flush();

    if (!stop)
        this_thread::sleep_for(pause);

    for (int i = 0; i < 1000000; ++i)
    {
        if (logger().threadStop())
            break;

        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "b1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "b2";
        log_debug2_m << "sssss o" << i + 1000000 << "b3";
    }

    cout << "Step 6 of 8" << endl;
    cout.flush();

    for (int i = 0; i < 1000000; ++i)
    {
        if (stop)
            break;

        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "c1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "c2";
        log_debug2_m << "sssss o" << i + 1000000 << "c3";
    }

    cout << "Step 7 of 8" << endl;
    cout.flush();

    for (int i = 0; i < 1000000; ++i)
    {
        if (stop)
            break;

        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "d1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "d2";
        log_debug2_m << "sssss o" << i + 1000000 << "d3";
    }

    cout << "Step 8 of 8" << endl;
    cout.flush();

    log_debug2 << "DONE " << mbstr;
    cout  << "End item test" << endl;
    if (!stop)
        cout  << "Elapsed: " << elapsedTimer.elapsed() << " ms" << endl;
    cout.flush();

    logger().flush();
    logger().waitingFlush();
    if (stop)
        return;

    cout  << "Flushed all data. Pause 30 sec." << endl;
    cout.flush();

    this_thread::sleep_for(chrono::milliseconds{5000});
    log_debug2_m << "null";
    this_thread::sleep_for(chrono::milliseconds{25000});
}

int main()
{
    signal(SIGTERM, &stopProgramHandler);
    signal(SIGINT,  &stopProgramHandler);

    logger().start();

    while (true)
    {
//        // Тест на нагрузочное тестирование
//        SaverFilePtr saver1(new SaverFile("alogger-test1", "/tmp/alogger-test1.log", Level::Debug2, false));
//        logger().addSaver(saver1);

//        SaverFilePtr saver2(new SaverFile("alogger-test2", "/tmp/alogger-test2.log", Level::Debug2, false));
//        logger().addSaver(saver2);

//        SaverFilePtr saver3(new SaverFile("alogger-test3", "/tmp/alogger-test3.log", Level::Debug2, false));
//        logger().addSaver(saver3);

        // Тест на утечку памяти (тест проводится долго, поэтому диск терзать незачем)
        SaverNullPtr saver1(new SaverNull("alogger-test1", Level::Debug2));
        logger().addSaver(saver1);

        SaverNullPtr saver2(new SaverNull("alogger-test2", Level::Debug2));
        logger().addSaver(saver2);

        SaverNullPtr saver3(new SaverNull("alogger-test3", Level::Debug2));
        logger().addSaver(saver3);

        if (stop)
            break;

        saveToLog();
    }

    logger().stop();
    return 0;
}
