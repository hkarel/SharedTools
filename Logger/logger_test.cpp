
#if 0
void logger_debug()
{
    #define log_error_m   logger().error_f  (__FILE__, __LINE__, "TetsModule: ")
    #define log_warn_m    logger().warn_f   (__FILE__, __LINE__, "TetsModule: ")
    #define log_info_m    logger().info_f   (__FILE__, __LINE__, "TetsModule: ")
    #define log_verbose_m logger().verbose_f(__FILE__, __LINE__, "TetsModule: ")
    #define log_debug_m   logger().debug_f  (__FILE__, __LINE__, "TetsModule: ")
    #define log_debug2_m  logger().debug2_f (__FILE__, __LINE__, "TetsModule: ")

    logger().start();
    //logger().addSaverStdOut(log::Level::DEBUG2);

    lblog::SaverCPtr saver(new lblog::SaverFile("lblogger-test", "/tmp/lblogger-test.log", lblog::Level::DEBUG2));
    logger().addSaver(saver);

    lblog::SaverCPtr saver2(new lblog::SaverFile("lblogger-test2", "/tmp/lblogger-test2.log", lblog::Level::DEBUG2));
    logger().addSaver(saver2);

    lblog::SaverCPtr saver3(new lblog::SaverFile("lblogger-test3", "/tmp/lblogger-test3.log", lblog::Level::DEBUG2));
    logger().addSaver(saver3);



    char mbstr[200] = {0};
    std::time_t t = std::time(NULL);
    std::strftime(mbstr, sizeof(mbstr), "%d.%m.%Y %H:%M:%S ", std::localtime(&t));
//        str += mbstr;


    for (int i = 0; i < 1000000; ++i)
    {
        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i << "a1";
        log_debug_m  << "bbbbb o" << i << "a2";
        log_debug2_m << "sssss o" << i << "a3";
    }

    //logger().stop();

    std::chrono::milliseconds dura( 10000 );
    std::this_thread::sleep_for( dura );
    std::cout << "====================================================" << std::endl;

    for (int i = 0; i < 1000000; ++i)
    {
        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "b1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "b2";
        log_debug2_m << "sssss o" << i + 1000000 << "b3";
    }

    for (int i = 0; i < 1000000; ++i)
    {
        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "c1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "c2";
        log_debug2_m << "sssss o" << i + 1000000 << "c3";
    }

    for (int i = 0; i < 1000000; ++i)
    {
        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "d1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "d2";
        log_debug2_m << "sssss o" << i + 1000000 << "d3";
    }

    std::this_thread::sleep_for( dura );
    std::cout << "====================================================" << std::endl;

    for (int i = 0; i < 1000000; ++i)
    {
        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "b1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "b2";
        log_debug2_m << "sssss o" << i + 1000000 << "b3";
    }

    for (int i = 0; i < 1000000; ++i)
    {
        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "c1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "c2";
        log_debug2_m << "sssss o" << i + 1000000 << "c3";
    }

    for (int i = 0; i < 1000000; ++i)
    {
        //log_error  << "aaaaa" << i;
        log_info_m   << "aaaaa o" << i + 1000000 << "d1";
        log_debug_m  << "bbbbb o" << i + 1000000 << "d2";
        log_debug2_m << "sssss o" << i + 1000000 << "d3";
    }


    log_debug2 << "DONE " << mbstr;
    std::cout << "DONE " << mbstr  << std::endl;;

    //sleep(120);
    //std::this_thread::sleep_for(std::chrono::seconds(60));
    std::chrono::milliseconds dura1( 6000000 );
    std::this_thread::sleep_for( dura1 );

    logger().stop();

    #undef log_error_m
    #undef log_warn_m
    #undef log_info_m
    #undef log_verbose_m
    #undef log_debug_m
    #undef log_debug2_m
}
#endif //#if 0

