
#include <tr1/functional>


template<typename T>
struct R
{
    T* value;
    T old;

    R(T* x, T newvalue)
      : value( x ), old( *x )
       {
           *value = newvalue;
       }

       ~R()
       {
           *value = old;
       }
};

struct R2
{
    std::tr1::function<void()> f;
    template<typename T>
    R2(T* x, T newvalue)
    {
        f = std::tr1::bind(&R2::j<T>, x, *x);
        *x = newvalue;
    }
    template<typename T>
    R2(volatile T* x, T newvalue)
    {
        f = std::tr1::bind(&R2::jv<volatile T>, x, *x);
        *x = newvalue;
    }


    R2() {f();}

    template<typename T>
    static void j(T* x, T y)
    {
        *x = y;
    }
    template<typename T>
    static void jv(volatile T* x, T y)
    {
        *x = y;
    }
};


struct R3
{
    struct RBase {virtual ~RBase() {} };

    template<typename T> struct R : RBase
    {
        R(T* ref, T val) {
            this->ref = ref;
            this->val = *ref;
            *ref = val;
        }
        ~R() {
            if (ref) *ref = val;
        }

        T* ref;
        T  val;
    };

    template<typename T> struct RVlt : RBase
    {
        RVlt(volatile T* ref, T val) {
            this->ref = ref;
            this->val = *ref;
            *ref = val;
        }
        ~RVlt() {
            if (ref) *ref = val;
        }

        volatile T* ref;
        T  val;
    };

    template<typename T>
    R3(T* ref, T value) : r(R<T>(ref, value)) {}

    template<typename T>
    R3(volatile T* ref, T value) : r(RVlt<T>(ref, value)) {}

    const RBase& r;
};


int a_ = 0;

struct A
{
    int a1;
    volatile int a2;

    A()
    {
        R2 r(&a_, 10);
        R2 r1(&a1, 10);
        R2 r2(&a2, 10);
    }

    A(int)
    {
        a1 = 0;
        a2 = 0;

        //printf()

        R3 r(&a_, 10);
        R3 r1(&a1, 10);
        R3 r2(&a2, 10);
    }


};


int main()
{
    A a;
    A a2(0);

    return 0;
}
