#include "ring_buffer.h"
#include <string.h>
#include <chrono>
#include <thread>
#include <string>
#include <vector>

using namespace std;

void testRingBuffer()
{

    char a5[6]; memset(a5, 'a', 5); a5[5] = {0};
    char b5[6]; memset(b5, 'b', 5); b5[5] = {0};
    char c5[6]; memset(c5, 'c', 5); c5[5] = {0};
    char d5[6]; memset(d5, 'd', 5); d5[5] = {0};
    char e5[6]; memset(e5, 'e', 5); e5[5] = {0};

    char ra[9];
    char rb[9];
    char rc[9];
    char rd[9];
    char re[9];

    RingBuffer rbuf;

//    // test 1
//    {
//        // Тест условия W1, W2
//        rbuf.reset();
//        rbuf1.init(18);

//        memset(ra, 0, 9);
//        memset(rb, 0, 9);
//        memset(rc, 0, 9);
//        memset(rd, 0, 9);

//        rbuf1.write(a5, 5);
//        rbuf1.write(b5, 5);
//        rbuf1.write(c5, 5);

//        rbuf1.read(ra, 5);
//        rbuf1.read(rb, 5);
//        rbuf1.read(rc, 5);

//        rbuf1.write(d5, 5);
//        rbuf1.read(rd, 5);
//    }

//    // test 2
//    {
//        // Тест условия W1
//        rbuf.reset();
//        rbuf2.init(15);

//        memset(ra, 0, 9);
//        memset(rb, 0, 9);
//        memset(rc, 0, 9);
//        memset(rd, 0, 9);

//        rbuf2.write(a5, 5);
//        rbuf2.write(b5, 5);
//        rbuf2.write(c5, 5);

//        rbuf2.read(ra, 5);
//        rbuf2.read(rb, 5);
//        rbuf2.read(rc, 5);

//        rbuf2.write(d5, 5);
//        rbuf2.read(rd, 5);

//        memset(ra, 0, 9);
//        memset(rb, 0, 9);
//        memset(rc, 0, 9);
//        memset(rd, 0, 9);

//        rbuf2.write(a5, 5);
//        rbuf2.write(b5, 5);
//        rbuf2.write(c5, 5);

//        rbuf2.read(ra, 5);
//        rbuf2.read(rb, 5);
//        rbuf2.read(rc, 5);

//    }

//    // test 3
//    {
//        // Тест условия W3
//        rbuf.reset();
//        rbuf.init(12);

//        memset(ra, 0, 9);
//        memset(rb, 0, 9);
//        memset(rc, 0, 9);
//        memset(rd, 0, 9);

//        rbuf.write(a5, 5);
//        rbuf.write(b5, 5);

//        rbuf.read(ra, 5);
//        rbuf.read(rb, 5);

//        rbuf.write(c5, 5);
//        rbuf.write(d5, 5);

//        rbuf.read(rc, 5);
//        rbuf.read(rd, 5);
//    }

//    // test 4
//    {
//        // Тест условия W4
//        rbuf.reset();
//        rbuf.init(12);

//        memset(ra, 0, 9);
//        memset(rb, 0, 9);
//        memset(rc, 0, 9);
//        memset(rd, 0, 9);
//        memset(re, 0, 9);

//        rbuf.write(a5, 5);
//        rbuf.write(b5, 5);

//        rbuf.read(ra, 5);
//        rbuf.read(rb, 5);

//        rbuf.write(c5, 5);
//        rbuf.write(d5, 5);
//        rbuf.write(e5, 5);

//        rbuf.read(rc, 5);
//        rbuf.read(rd, 5);
//    }

//    // test 5
//    {
//        // Тест условия W4
//        rbuf.reset();
//        rbuf.init(12);

//        memset(ra, 0, 9);
//        memset(rb, 0, 9);
//        memset(rc, 0, 9);
//        memset(rd, 0, 9);
//        memset(re, 0, 9);

//        rbuf.write(a5, 5);
//        rbuf.write(b5, 5);

//        rbuf.read(ra, 2);
//        //rbuf.read(rb, 5);

//        rbuf.write(c5, 5);
//        //rbuf.write(d5, 5);
//        //rbuf.write(e5, 5);

//        //rbuf.read(ra, 5);
//        //rbuf.read(rb, 3);
//    }

    // test 6
    vector<string> result;
    {
        rbuf.reset();
        rbuf.init(12);
        int rindex = 0;

        auto rb_write = [&]()
        {
            for (int i = 0; i < 100; ++i)
                for (int j = 0; j < 5; ++j)
                {
                    if (j == 0)
                        rbuf.write(a5, 5);
                    else if (j == 1)
                        rbuf.write(b5, 5);
                    else if (j == 2)
                        rbuf.write(c5, 5);
                    else if (j == 3)
                        rbuf.write(d5, 5);
                    else if (j == 4)
                        rbuf.write(e5, 5);

                    static chrono::milliseconds sleepThread {10};
                    this_thread::sleep_for(sleepThread);
                }
        };

        auto rb_read = [&]()
        {
            static chrono::milliseconds sleepThread {5};
            this_thread::sleep_for(sleepThread);

            char r[6]; r[5] = '\0';
            while (rindex < 100)
            {
                if (rbuf.read(r, 5))
                {
                    ++rindex;
                    result.push_back(string(r));
                }
            }
        };
        thread t1 {rb_write};
        thread t2 {rb_read};

        t1.join();
        t2.join();

    }

    return;
}
