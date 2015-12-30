/* clang-format off */
/**
  Closure library 1.1 sample file.
  file "test_closure.cpp"

  Written by Ivan Yankov aka _Winnie (woowoowoow@bk.ru)
  Many thanks to Wolfhound
*/

#include <vector>
#include <iostream>

#include "closure.h"

#include <iostream>

void f(Closure<float()> c)
{
  std::cout <<c() <<'\n';
}

void f(Closure<float(float)> c)
{
  std::cout <<c(3)<<'\n';
}

void f(Closure<float(float, float)> c)
{
  std::cout <<c(3,4)<<'\n';
}


struct Test
{
  float F0() { return 3; }
  float FC0() const { return 33; }

  float F(float x) { return x+1; }
  float FC(float x) const { return x*2; }

  float F2(float x, float y) { return x+y; }
  float FC2(float x, float y) const { return x*y; }

  float XXX(Closure<float(float,float)> c, double x, int y) const { return c(x,y)+300; }
};


#include <assert.h>



int main()
{
  Test test= Test();

  {
    //test conversion to bool
    Closure<float(float,float)> x2;

    if (x2)
      assert(false);

    if (!!x2)
      assert(false);

    if (x2 != NULL)
      assert(false);

    assert(!x2);

    x2 = CLOSURE(&test, &Test::FC2);

    if (!x2)
      assert(false);

    if (x2 == NULL)
      assert(false);

    assert(x2);
  }

  f(CLOSURE(&test, &Test::F0));
  f(CLOSURE(&test, &Test::FC0));
  f(CLOSURE(&test, &Test::F));
  f(CLOSURE(&test, &Test::FC));
  f(CLOSURE(&test, &Test::F2));
  f(CLOSURE(&test, &Test::FC2));
  f(CLOSURE(&test, &Test::FC2));
  Closure<float(Closure<float(float,float)>, double, int)> xxx = CLOSURE(&test, &Test::XXX);
  std::cout <<xxx(CLOSURE(&test, &Test::F2), 1,20);

  std::cout << '\n';

  return 0;
}
