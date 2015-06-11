/****************************************************************************
  Прототипом модуля является Closure library 1.1 созданная
  Ivan Yankov aka _Winnie (woowoowoow@bk.ru)

  Исходное решение переработано под механизм typename...
  Автор переработанного решения: Карелин Павел (hkarel), hkarel@yandex.ru

  В модуле представлен аналог функции std::bind. Исходное решение (Closure library)
  было придумано в 2004 году, когда полноценной std::bind еще не существовало.
  В отличие от std::bind данное решение более примитивное, как с точки зрения
  реализации, так и с точки зрения связывания: сигнатуры функций должны совпадать.
  К достоинствам можно отнести следующие аспекты:
    - память выделяется только на стеке (не используется new());
    - не хранится значение указателя на связываемую функцию (т.е. в отличии от
      аналогичных решений, например FastDelegate, не требует учитывать особен-
      ности работы с указателями на функции на разных платформах и на разных
      компиляторах);
    - сигнатуры функций должны совпадать, как следствие, форма записи связывания
      CLOSURE более простая и наглядная. Подобное ограничение не допускает связы-
      вания в стиле std::bind, что в некоторой степени делает код более строгим.

  Вероятно, название CLOSURE происходит от директивы __closure  компилятора
  Borland C++Builder, которая выполняет аналогичную функцию.

****************************************************************************/

#pragma once

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif


// Обобщенная декларация
template <typename, typename... > class Closure;


namespace closure
{
    // Forward декларации
    template <typename, typename, typename... > struct CreateHelper;
    template <typename, typename, typename... > struct CreateHelperConst;

    template <typename R, typename... Args>
    constexpr Closure<R (Args...)> Create(R (*)(Args...));

} // namespace closure


/**
  Базовый класс позволяет иметь общие операторы '==' и '!='
*/
class ClosureBase
{
protected:
    void* self  = {0};
    void* proxy = {0};

public:
    bool equal(const ClosureBase& c) const NOEXCEPT
    {
        return (self == c.self) && (proxy == c.proxy);
    }
};

inline bool operator== (const ClosureBase& c1, const ClosureBase& c2) NOEXCEPT
{
    return c1.equal(c2);
}

inline bool operator!= (const ClosureBase& c1, const ClosureBase& c2) NOEXCEPT
{
    return !c1.equal(c2);
}

/**
  Closure
*/
template <typename R, typename... Args>
class Closure<R (Args...)> : public ClosureBase
{
public:
    typedef R (*proxy_m)(void*, Args...);
    typedef R (*proxy_f)(Args...);

    Closure() = default;
    Closure(const Closure &) = default;
    Closure& operator= (const Closure &) = default;

    Closure(Closure &&) = default;
    Closure& operator= (Closure &&) = default;

    R operator() (Args... args)
    {
        return (self)
               ? ( (proxy) ? ((proxy_m)proxy)(self, args...) : R() )
               : ( (proxy) ? ((proxy_f)proxy)(args...)       : R() );
    }

    R operator() (Args... args) const
    {
        return (self)
               ? ( (proxy) ? ((proxy_m)proxy)(self, args...) : R() )
               : ( (proxy) ? ((proxy_f)proxy)(args...)       : R() );
    }

    bool empty() const NOEXCEPT {return (proxy == 0);}

    explicit operator bool () const NOEXCEPT {return !empty();}
    bool operator! () const NOEXCEPT {return empty();}

    void reset(const Closure& c = Closure()) {*this = c;}

private:
    Closure(void *self, proxy_m proxy) NOEXCEPT
    {
        this->self = self;
        this->proxy = (void*)proxy;
    }

    Closure(proxy_f proxy) NOEXCEPT
    {
        this->self = 0;
        this->proxy = (void*)proxy;
    }

    template <typename, typename, typename... >
    friend struct closure::CreateHelper;

    template <typename, typename, typename... >
    friend struct closure::CreateHelperConst;

    template <typename R0, typename... Args0>
    friend constexpr Closure<R0 (Args0...)> closure::Create(R0 (*)(Args0...));
};


namespace closure
{
    template <typename... Args>
    struct ProxyFunc
    {
        template <typename T, typename R, R (T::*mem_func)(Args...)>
        static R Func(void* self, Args... args)
        {
            return (static_cast<T*>(self) ->* mem_func)(args...);
        }

        template <typename T, typename R, R (T::*mem_func)(Args...) const>
        static R Func(void* self, Args... args)
        {
            return (static_cast<T*>(self) ->* mem_func)(args...);
        }
    };

    template <typename B, typename R, typename... Args>
    struct CreateHelper
    {
        //typedef R (B::*func_type)(Args...);
        //typedef Closure<R (Args...)> ClosureType;

        template <R (B::*mem_func)(Args...), typename D>
        Closure<R (Args...)> Init(D *d) NOEXCEPT
        {
            // Проверяем корректность преобразования типа. Допускается преобразование
            // только от классов-наследников к базовым классам.
            B* b = d;
            return Closure<R (Args...)>(b, ProxyFunc<Args...>::template Func<B, R, mem_func>);
        }
    };

    template <typename B, typename R, typename... Args>
    struct CreateHelperConst
    {
        template <R (B::*mem_func)(Args...) const, typename D>
        Closure<R (Args...)> Init(D *d) NOEXCEPT
        {
            B* b = d;
            return Closure<R (Args...)>(b, ProxyFunc<Args...>::template Func<B, R, mem_func>);
        }
    };

    template <typename T, typename R, typename... Args>
    constexpr CreateHelper<T, R, Args...> Create(R (T::*f)(Args...))
    {
        return CreateHelper<T, R, Args...>();
    }

    template <typename T, typename R, typename... Args>
    constexpr CreateHelperConst<T, R, Args...> Create(R (T::*)(Args...) const)
    {
        return CreateHelperConst<T, R, Args...>();
    }

    template <typename R, typename... Args>
    constexpr Closure<R (Args...)> Create(R (*f)(Args...))
    {
        return Closure<R (Args...)>(f);
    }

} // namespace closure

// Для функций членов класса
#define CLOSURE(PTR, MEM_PTR) (closure::Create(MEM_PTR).Init<MEM_PTR>(PTR))

// Для обычных функций
#define CLOSUREF(FUNC) (closure::Create(FUNC))

