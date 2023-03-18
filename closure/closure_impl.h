/* clang-format off */
/**
  Closure library 1.1
  file "closure_impl.h"

  Written by Ivan Yankov aka _Winnie (woowoowoow@bk.ru)
  Many thanks to Wolfhound

  Modyfied:  Karelin Pavel (hkarel), hkarel@yandex.ru
*/

#ifndef CLOSURE_BASE
#define CLOSURE_BASE

namespace closure
{
    struct ClosureBase
    {
        void* self;
        void* proxy;
    };

    inline bool operator== (const ClosureBase& c1, const ClosureBase& c2)
    {
        return (c1.self == c2.self) && (c1.proxy == c2.proxy);
    }

    inline bool operator!= (const ClosureBase& c1, const ClosureBase& c2)
    {
        return (c1.self != c2.self) || (c1.proxy != c2.proxy);
    }

} //namespace closure

#endif //#ifndef CLOSURE_BASE



#ifndef CLOSURE_COMMA
#define CLOSURE_COMMA ,
#endif


template<typename> struct Closure;


namespace closure
{
    //this template generate function, which call "pmem" member function
    template<typename T, typename R CLOSURE_COMMA CLOSURE_TYPENAMES, R (T::*mem_func)(CLOSURE_TYPES)>
    R ProxyFunc(void* self CLOSURE_COMMA CLOSURE_FUNC_PARAMS)
    {
        return (static_cast<T*>(self) ->* mem_func)(CLOSURE_FUNC_ARGS);
    }

    //this template generate function, which call "pmem" const member function
    template<typename T, typename R CLOSURE_COMMA CLOSURE_TYPENAMES, R (T::*mem_func)(CLOSURE_TYPES) const>
    R ProxyFuncConst(void* self CLOSURE_COMMA CLOSURE_FUNC_PARAMS)
    {
        return (static_cast<T*>(self) ->* mem_func)(CLOSURE_FUNC_ARGS);
    }

} //namespace closure


template<typename R CLOSURE_COMMA CLOSURE_TYPENAMES>
class Closure<R(CLOSURE_TYPES)> : private closure::ClosureBase
{
public:
    typedef R (*proxy_t)(void* CLOSURE_COMMA CLOSURE_TYPES);

    Closure()
    {
        self = 0;
        //this->proxy = static_cast<void*>(static_cast<proxy_t>(
        //               closure::CLOSURE_NAMESPACE::ProxyFuncDummy<R CLOSURE_COMMA CLOSURE_TYPES>));
        //this->proxy = reinterpret_cast<void*>(static_cast<proxy_t>(
        //                 closure::CLOSURE_NAMESPACE::ProxyFuncDummy<R CLOSURE_COMMA CLOSURE_TYPES>));
        proxy = 0;
    }

    //initialization whith proxy function "p_proxy_function" and context "p_this".
    //signature of p_proxy_function should match template parameters of Closure<...>
    //and take additional parameter void *
    Closure(void *self, proxy_t proxy)
    {
        this->self = self;
        this->proxy = (void*)(proxy);
    }

    R operator()(CLOSURE_FUNC_PARAMS)
    {
        //return (static_cast<proxy_t>(proxy))(self CLOSURE_COMMA CLOSURE_FUNC_ARGS);
        return (proxy) ? ((proxy_t)(proxy))(self CLOSURE_COMMA CLOSURE_FUNC_ARGS) : R();
    }

    R operator() (CLOSURE_FUNC_PARAMS) const
    {
        //return (static_cast<proxy_t>(proxy))(self CLOSURE_COMMA CLOSURE_FUNC_ARGS);
        return (proxy) ? ((proxy_t)(proxy))(self CLOSURE_COMMA CLOSURE_FUNC_ARGS) : R();
    }

    bool empty() const {return (proxy == 0) || (self == 0);}
    bool is_empty() const {return empty();}
    bool operator!() const {return empty();}

    // Karelin: Такого решения вполне достаточно для имитации safe_bool
    typedef /*bool*/ proxy_t safe_bool;
    operator safe_bool() const
    {
        return empty() ? 0 : reinterpret_cast<safe_bool>(1);
    }

    void reset(const Closure& c = Closure()) {*this = c;}
};


namespace closure
{
    namespace CLOSURE_NAMESPACE
    {
        template<typename B, typename R CLOSURE_COMMA CLOSURE_TYPENAMES>
        struct CreateClosureHelper
        {
            template<R (B::*mem_func)(CLOSURE_TYPES), typename D>
            Closure<R(CLOSURE_TYPES)> Init(D *d)
            {
                // Проверяем корректность преобразования типа. Допускается преобразование
                // только от классов-наследников к базовым классам.
                B* b = d;
                Closure<R(CLOSURE_TYPES)> closure(
                    b, ProxyFunc<B, R CLOSURE_COMMA CLOSURE_TYPES, mem_func>);
                return closure;
            }
        };

        template<typename B, typename R CLOSURE_COMMA CLOSURE_TYPENAMES>
        struct CreateClosureHelperConst
        {
            template<R (B::*mem_func)(CLOSURE_TYPES) const, typename D>
            Closure<R(CLOSURE_TYPES)> Init(const D *d)
            {
                const B* b = d;
                Closure<R(CLOSURE_TYPES)> closure(
                    //const_cast<void*>(static_cast<const void*>(p_b_this)),
                    const_cast<B*>(b), ProxyFuncConst<B, R CLOSURE_COMMA CLOSURE_TYPES, mem_func>);
                return closure;
            }
        };

    } //namespace CLOSURE_NAMESPACE


    //helper function, to deduce return and parameters types of given pointer to member function
    template<typename T, typename R CLOSURE_COMMA CLOSURE_TYPENAMES>
    CLOSURE_NAMESPACE::CreateClosureHelper<T, R CLOSURE_COMMA CLOSURE_TYPES>
        CreateClosure(R (T::*)(CLOSURE_TYPES))
    {
        return CLOSURE_NAMESPACE::CreateClosureHelper<T, R CLOSURE_COMMA CLOSURE_TYPES>();
    }

    //helper function, to deduce return and parameters types of given pointer to member const function
    template<typename T, typename R CLOSURE_COMMA CLOSURE_TYPENAMES>
    CLOSURE_NAMESPACE::CreateClosureHelperConst<T, R CLOSURE_COMMA CLOSURE_TYPES>
        CreateClosure(R (T::*)(CLOSURE_TYPES) const)
    {
        return CLOSURE_NAMESPACE::CreateClosureHelperConst<T, R CLOSURE_COMMA CLOSURE_TYPES>();
    }

} //namespace closure



#undef CLOSURE_TYPENAMES
#undef CLOSURE_TYPES
#undef CLOSURE_FUNC_PARAMS
#undef CLOSURE_FUNC_ARGS
#undef CLOSURE_COMMA
#undef CLOSURE_NAMESPACE

