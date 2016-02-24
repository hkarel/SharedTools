/* clang-format off */
//#define QT_CORE_LIB

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <utility>
#include "../container_ptr.h"

// g++ container_ptr_utest.cpp -I/usr/include/qt4 -I/usr/include/qt4/QtCore
// clang++ -std=c++11 -ggdb3 container_ptr_utest.cpp -DCONTAINER_PTR_DEBUG -o container_ptr_utest

// CentOS 6
// clang++ -v -std=c++11 -ggdb3 container_ptr_utest.cpp -DCONTAINER_PTR_DEBUG -I/usr/include/c++/4.6 -I/usr/include/c++/4.6/x86_64-linux-gnu -L/usr/lib/gcc/x86_64-redhat-linux/4.4.4  -o container_ptr_utest
// clang++ -v -std=c++11 -ggdb3 container_ptr_utest.cpp -DCONTAINER_PTR_DEBUG -I/usr/include/c++/4.7 -I/usr/include/x86_64-linux-gnu/c++/4.7 -L/usr/lib/gcc/x86_64-redhat-linux/4.4.4  -o container_ptr_utest


struct A {
    A() {}
    A(int a) {
        this->a = a;
    }
    //A(const A& A_) = delete;
    //A& operator= (const A&) = delete;
    A(const A& A_) {
        a = A_.a;
    }
    A(A&& A_) {
        a = A_.a;
        A_.a = -1;
    }
    ~A() {
    }
    int a = {5};
};
struct B : A {};
struct C {};

struct D
{
    D() {}
    //D(const A& a) = delete;
    D(const A& a) : a(a) {
    }
    //D(const D& d) = delete;
    D(const D& d) : a(d.a) {
    }
    D(A&& a) : a(std::forward<A>(a)) {
    }
    D(D&& d) : a(std::forward<A>(d.a)) {
    }
    ~D() {
    }
    A a;
};


template<typename T> struct allocator_ptr_m
{
    static T* create() {return new T();}
    static T* create(const T* x) {return (x) ? new T(*x) : new T();}
    static void destroy(T* x) {delete x;}
    //static void destroy(T* x, bool join) {if (x) {if (join) x->~T(); else delete x;}}
};



template<typename T> struct other_allocator_ptr
{
    inline static T* create() {return new T();}
    inline static T* create(const T* x) {return (x) ? new T(*x) : new T();}
    inline static void destroy(T* x) {delete x;}
    //inline static void destroy(T* x, bool /*join*/) {if (x) x->~T();}
};



typedef container_ptr<A, allocator_ptr_m> a_ptr_t;
typedef container_ptr<A, other_allocator_ptr> a_other_ptr_t;

typedef container_ptr<B, allocator_ptr_m> b_ptr_t;
typedef container_ptr<C, allocator_ptr_m> c_ptr_t;

a_ptr_t create_ptr()
{
    a_ptr_t a(new A());
    //return static_cast<a_ptr_t&&>(a);
    return a;
    //return std::forward<a_ptr_t>(a);
    //return a_ptr_t();
}

int main()
{
//     std::cout << __GNUC__ * 100 << "\n";
//     std::cout << __GNUC_MINOR__ * 10 << "\n";
//     std::cout << __GNUC__ * 100 + __GNUC_MINOR__ * 10 << "\n";
//
// #ifdef __clang__
//     std::cout << __clang__  << "\n";
//     std::cout << __clang_major__  << "\n";
//     std::cout << __clang_minor__  << "\n";
// #endif

    a_ptr_t a(new A());
    b_ptr_t b(new B());
    //a_ptr_t a2 = a;
    a_other_ptr_t oa(new A);

    //a_ptr_t d = oa;
    //a_ptr_t a2 = a;
    //a_ptr_t a2 = static_cast<a_ptr_t&&>(a);
    //a_ptr_t a2 = std::forward<a_ptr_t>(a);
    a_ptr_t a3 = a;

    a_ptr_t a4 = create_ptr();
    //a_ptr_t a5 = std::forward<a_ptr_t>(a4);

    a = b;
    //b = a;
    std::cout << (b.empty() ? "empty" : "not empty") << "\n";
    //a = std::forward<a_ptr_t>(b);
    //a = std::move<a_ptr_t>(b);
    //a = std::forward<a_ptr_t>(a4);
    a = static_cast<b_ptr_t&&>(b);
    std::cout << (b.empty() ? "empty" : "not empty") << "\n";

    a.reset();
    //---

    container_ptr<D> d1 = container_ptr<D>::create_join_ptr(A(1));

    A aa(2);
    container_ptr<D> d2 = container_ptr<D>::create_join_ptr(aa);

    const A aa_const(3);
    container_ptr<D> d3 = container_ptr<D>::create_join_ptr(aa_const);



    //create_ptr();

    //b_ptr_t b2(a);
    //b = arr;
    //arr = b;

//    //A* a1 = new A();
//    //B* b1 = a1;

//    c_ptr_t c(new C());
//    //a = c;



    //std::cout << "hello\n";
    return 0;
}
