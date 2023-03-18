/* clang-format off */
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

void f(Closure<float (const float&, const float*)> c)
{
    float y = 4;
    std::cout << c(3, &y) << '\n';
}


struct TestAbstract
{
    virtual float F0() = 0;
    virtual float FC0() const = 0;
};


struct Test : TestAbstract
{
    float operator() (float x, float y) {return x + y;}

    float F0() override {return 3;}
    float FC0() const override {return 33;}

    float F(float x) {return x + 1;}
    float FC(float x) const {return x * 2;}

    float F2(float x, float y) {return x + y;}
    float FC2(float x, float y) const {return x * y;}

    float F3(float&& x, const float* y) {return x + *y + 3;}
    float FC3(float&& x, const float* y) const {return x + *y + 3;}

    float F4(const float&& x, const float* y) {return x + *y + 4;}
    float FC4(const float&& x, const float* y) const {return x + *y + 4;}

    virtual float F5(const float& x, const float* y) {return -1;}
    virtual float FC5(const float& x, const float* y) const {return -1;}

    //float XXX(Closure<float (float,float)> c, double x, int y) const {return c(x,y)+300;}
    float XXX(Closure<float (float,float)> c, double x, int y) {return c(x, y) + 300;}
};

float F5(float x, float y) {return x + y + 5;}
float F6(float&& x, float y) {return x + y + 6;}
float F7(float&& x, const float* y) {return x + *y + 6;}
//constexpr float (Test::*f)() = &Test::F0;

struct TestDeriv : Test
{
    float F5(const float& x, const float* y) override {return x + *y + 5;}
    float FC5(const float& x, const float* y) const override {return x + *y + 5;}
};

//--- Эксперименты (неудачные) по вызову lambda функции ---
#include <type_traits>

auto l = [](float x, float y) -> float
{
    return x + y;
};

typedef float (*l_ptr)(float, float);
l_ptr l_func = l;

float (*l_func2)(float, float) = l;

// Обобщенная декларация
template<typename> struct CreateHelperL {};

template<typename R, typename... Args>
struct CreateHelperL<R (Args...)>
{
    static int test() {return 0;}

};

template<typename R, typename... Args>
struct CreateHelperL<R (*)(Args...)>
{
    static int test() {return 1;}

};

template<typename B, typename R, typename... Args>
struct CreateHelperL<R (B::*)(Args...)>
{
    static int test() {return 2;}
};

template<typename Func, Func func>
void CreateL()
{
    //return 0;
    //CreateHelperL<Func> aa;
    //CreateHelperL<Func>::template Init<func>();
    std::cout << CreateHelperL<Func>::test() << '\n';

    //return (*func)(3, 5);
    //return CreateHelper<Func>::template Init<func>();
    //return 0;
}

template<typename Func, typename... Args>
void CreateL2(Func(Args...))
{
    //return 0;
    //CreateHelperL<Func> aa;
    //CreateHelperL<Func>::template Init<func>();
    std::cout << CreateHelperL<Func>::test() << '\n';

    //return (*func)(3, 5);
    //return CreateHelper<Func>::template Init<func>();
    //return 0;
}
//--- Эксперименты (неудачные) по вызову lambda функции ---


int main()
{
//    //--- Эксперименты (неудачные) по вызову lambda функции ---

//    Test t;

//    auto l2 = [=](float x, float y) -> float
//    {
//        return x + y;
//    };
//    (void) l2;

//    //typedef std::decay<decltype(&l)>::type tt;

//    CreateL<decltype(&l::operator()), &l::operator()>();
//    //CreateL<decltype(&l), &l>();
//    //CreateL<std::decay<decltype(l)>::type, &l>();
//    //CreateL2( l);
//    //CreateL<decltype(&l_func2), &l_func2>();
//    //CreateL<decltype(&t), &t>();
//    CreateL<decltype(&Test::operator()), &Test::operator()>();
//    CreateL<decltype(&Test::F2), &Test::F2>();
//    CreateL<decltype(&F5), &F5>();
//    std::cout << "---\n";

//    //CreateL<decltype(&F5), &F5>();
//    std::cout << std::is_function<decltype(F5)>::value << '\n';
//    std::cout << std::is_function<decltype(&F5)>::value << '\n';
//    std::cout << std::is_function<std::remove_pointer<decltype(&F5)>::type>::value << '\n';
//    std::cout << "---\n";

//    std::cout << std::is_function<decltype(&Test::FC2)>::value << '\n';
//    std::cout << std::is_function<decltype(&Test::operator ())>::value << '\n';
//    std::cout << std::is_function<std::remove_pointer<decltype(&Test::FC2)>::type>::value << '\n';
//    std::cout << "---\n";

//    std::cout << std::is_function<decltype(l)>::value << '\n';

//    return 0;
//    //------------------------------------------------------------------------

    Test test;
    TestDeriv test_deriv;

    {
        //test conversion to bool
        //bool a = true;
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

    TestAbstract* test_abstract = &test;

    f(CLOSURE(&TestAbstract::F0,  test_abstract));  // Вывод 3
    f(CLOSURE(&TestAbstract::FC0, test_abstract));  // Вывод 33
    
    f(CLOSURE(&Test::F0,  &test));               // Вывод 3
    f(CLOSURE(&Test::FC0, &test));               // Вывод 33
    f(CLOSURE(&Test::F,   &test));               // Вывод 4
    f(CLOSURE(&Test::FC,  &test));               // Вывод 6
    f(CLOSURE(&Test::F2,  &test));               // Вывод 7
    f(CLOSURE(&Test::FC2, &test));               // Вывод 12

    // Вызов виртуальных функций
    f(CLOSURE(&Test::F5,  &test));               // Вывод -1
    f(CLOSURE(&Test::FC5, &test));               // Вывод -1
    f(CLOSURE(&Test::F5,  (Test*)&test_deriv));  // Вывод 12
    f(CLOSURE(&Test::FC5, (Test*)&test_deriv));  // Вывод 12
    f(CLOSURE(&TestDeriv::F5,  &test_deriv));    // Вывод 12
    f(CLOSURE(&TestDeriv::FC5, &test_deriv));    // Вывод 12

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
    std::cout << "Test::F6: " << f6(1, 20)  << '\n';

    Closure<float (float&&, const float*)> f7 = CLOSURE(&F7);
    float y7 = 20;
    std::cout << "Test::F7: " << f7(1, &y7)  << '\n';


    // Failed compilation test
    //CLOSURE(&Test::F2, &test, &test);

    std::cout << '\n';

    return 0;
}
