/**
  Closure library 1.1 sample file.
  file "test_closure.cpp"

  Written by Ivan Yankov aka _Winnie (woowoowoow@bk.ru)
  Many thanks to Wolfhound
*/

// g++ -std=c++11 -ggdb3  test_closure3.cpp -o test_closure3

#include <typeinfo>
#include <vector>
#include <tuple>
#include <iostream>
#include <functional>
#include <assert.h>

#include "closure3.h"

using namespace std;


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

    float F(float x) {return x + 1;}
    float FC(float x) const {return x * 2;}

    float F2(float x, float y) {return x + y;}
    float FC2(float x, float y) const {return x * y;}

    float F3(float&& x, const float* y) {return x + *y + 3;}
    float FC3(float&& x, const float* y) const {return x + *y + 3;}

    float F4(const float&& x, const float* y) {return x + *y + 4;}
    float FC4(const float&& x, const float* y) const {return x + *y + 4;}


    //float XXX(Closure<float (float,float)> c, double x, int y) const {return c(x,y)+300;}
    float XXX(Closure<float (float,float)> c, double x, int y) {return c(x, y) + 300;}
};

float F5(float x, float y) {return x + y + 5;}
float F6(float&& x, float y) {return x + y + 6;}
float F7(float&& x, const float* y) {return x + *y + 6;}
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

        x2 = CLOSURE(&Test::FC2, &test);

        // fail compilation, restriction C++11: explicit operator bool()
        // assert(x2 == a);

        if (!x2)
            assert(false);

        // fail compilation, restriction C++11: explicit operator bool()
        // if (x2 == NULL)
        //     assert(false);

        assert(x2);

    }


    f(CLOSURE(&Test::F0,  &test));
    f(CLOSURE(&Test::FC0, &test));
    f(CLOSURE(&Test::F,   &test));
    f(CLOSURE(&Test::FC,  &test));
    f(CLOSURE(&Test::F2,  &test));
    f(CLOSURE(&Test::FC2, &test));
    f(CLOSURE(&Test::FC2, &test));

    float f = 20;
    const float fc = 20;

    Closure<float (float&&, const float*)> tf3 = CLOSURE(&Test::F3, &test);
    std::cout << "Test::F3: " << tf3(1, &fc) << '\n';
    std::cout << "Test::F3: " << tf3(std::move(f), &fc) << '\n';

    Closure<float (float&&, const float*)> tfc3 = CLOSURE(&Test::FC3, &test);
    std::cout << "Test::FC3: " << tfc3(1, &fc) << '\n';

    Closure<float (const float&&, const float*)> tf4 = CLOSURE(&Test::F4, &test);
    std::cout << "Test::F4: " << tf4(1, &fc) << '\n';
    std::cout << "Test::F4: " << tf4(std::move(f), &fc) << '\n';

    Closure<float (const float&&, const float*)> tfc4 = CLOSURE(&Test::FC4, &test);
    std::cout << "Test::FC4: " << tfc4(1, &fc) << '\n';

    Closure<float (Closure<float (float,float)>, double, int)> xxx = CLOSURE(&Test::XXX, &test);
    std::cout << "Test::XXX(Test::F2): " << xxx(CLOSURE(&Test::F2, &test), 1, 20) << '\n';

    Closure<float (float,float)> f5 = CLOSURE(&F5);
    std::cout << "Test::XXX(F5): " << xxx(f5, 1, 20)  << '\n';

    Closure<float (float&&, float)> f6 = CLOSURE(&F6);
    std::cout << "Test::XXX(F6): " << f6(1, 20)  << '\n';

    // Failed compilation test
    //CLOSURE(&Test::F2, &test, &test);

    std::cout << '\n';

    return 0;
}
