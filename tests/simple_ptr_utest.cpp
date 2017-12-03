/* clang-format off */
//#include <stdio.h>
#include <iostream>
#include "../simple_ptr.h"


struct A {};
struct B : A {};
struct C {};



typedef simple_ptr<A, allocator_ptr> a_ptr_t;
typedef simple_ptr<A, allocator_array_ptr> a_arr_ptr_t;

typedef simple_ptr<B, allocator_ptr> b_ptr_t;
typedef simple_ptr<C, allocator_ptr> c_ptr_t;



int main()
{
    a_ptr_t a(new A());
    //a_ptr_t a2 = a;
    a_arr_ptr_t arr(new A[5]);

    //a_ptr_t d = arr;
    a_ptr_t d2 = a;
    //a = arr;

    b_ptr_t b(new B());
    a = b;

    //b_ptr_t b2(a);
    //b = arr;
    //arr = b;

    //A* a1 = new A();
    //B* b1 = a1;

    c_ptr_t c(new C());
    //a = c;



    //std::cout << "hello\n";
    return 0;
}
