/* clang-format off */
/**
  Closure library 1.1 sample file.
  file "test_closure.cpp"

  Written by Ivan Yankov aka _Winnie (woowoowoow@bk.ru)
  Many thanks to Wolfhound
*/

// g++ -std=c++11 -ggdb3  test_closure2.cpp -o test_closure2

#include <vector>
#include <iostream>
#include <assert.h>

#include "closure2.h"


void f(Closure<float ()> c)
{
    std::cout << c() << '\n';
}

void f(Closure<float (float)> c)
{
    std::cout << c(3) << '\n';
}

void f(Closure<float (float, float)> c)
{
    std::cout << c(3, 4) << '\n';
}


struct Test
{
    float F0() {return 3;}
    float FC0() const {return 33;}

    float F(float x) {return x+1;}
    float FC(float x) const {return x*2;}

    float F2(float x, float y) {return x+y;}
    float FC2(float x, float y) const {return x*y;}

    float XXX(Closure<float (float,float)> c, double x, int y) const {return c(x,y)+300;}
};

float F5(float x, float y) {return x+y+5;}
//constexpr float (Test::*f)() = &Test::F0;


int main()
{
    Test test;

    {
        //test conversion to bool
        bool a = true;
        Closure<float (float,float)> x2;

        // fail compilation, restriction C++11: explicit operator bool()
        // assert(x2 != a);


        if (x2)
            assert(false);

        if (!!x2)
            assert(false);

        // fail compilation, restriction C++11: explicit operator bool()
        // if (x2 != NULL)
        //     assert(false);

        assert(!x2);

        x2 = CLOSURE(&test, &Test::FC2);

        // fail compilation, restriction C++11: explicit operator bool()
        // assert(x2 == a);

        if (!x2)
            assert(false);

        // fail compilation, restriction C++11: explicit operator bool()
        // if (x2 == NULL)
        //     assert(false);

        assert(x2);

    }


    f(CLOSURE(&test, &Test::F0));
    f(CLOSURE(&test, &Test::FC0));
    f(CLOSURE(&test, &Test::F));
    f(CLOSURE(&test, &Test::FC));
    f(CLOSURE(&test, &Test::F2));
    f(CLOSURE(&test, &Test::FC2));
    f(CLOSURE(&test, &Test::FC2));
    Closure<float (Closure<float (float,float)>, double, int)> xxx = CLOSURE(&test, &Test::XXX);
    std::cout << xxx(CLOSURE(&test, &Test::F2), 1, 20) << '\n';

    //Closure<float(float,float)> f = closure::Create(&F5);
    Closure<float (float,float)> ff = CLOSUREF(&F5);
    std::cout << xxx(ff, 1, 20)  << '\n';

    // lambda test
    Closure<float (float,float)> l;
    {
        float (*lm)(float,float) =
            [](float, float) -> float {
                std::cout << "lambda" << '\n';
                return 0;
            };

        l  = CLOSUREF(lm);
    }
    l(1, 1);

    std::cout << '\n';

    return 0;
}
