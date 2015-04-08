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
    - возможность использовать операторы '==' и '!=' (std::function не имеет
      такой возможности). Следствием этого являются более простые, в отличии
      от std::function, реализации событийных механизмов (см. simple_signal).

  Вероятно, название CLOSURE происходит от директивы __closure  компилятора
  Borland C++Builder, которая выполняет аналогичную функцию.

****************************************************************************/

#pragma once
#include <utility>


namespace closure
{
    // Forward декларация
    template <typename> struct CreateHelper;

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
    bool equal(const ClosureBase& c) const noexcept
    {
        return (self == c.self) && (proxy == c.proxy);
    }
};

inline bool operator== (const ClosureBase& c1, const ClosureBase& c2) noexcept
{
    return c1.equal(c2);
}

inline bool operator!= (const ClosureBase& c1, const ClosureBase& c2) noexcept
{
    return !c1.equal(c2);
}

// Обобщенная декларация
template <typename, typename... > class Closure;


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
               ? ( (proxy) ? ((proxy_m)proxy)(self, std::forward<Args>(args)...) : R() )
               : ( (proxy) ? ((proxy_f)proxy)(std::forward<Args>(args)...)       : R() );
    }

    R operator() (Args... args) const
    {
        return (self)
               ? ( (proxy) ? ((proxy_m)proxy)(self, std::forward<Args>(args)...) : R() )
               : ( (proxy) ? ((proxy_f)proxy)(std::forward<Args>(args)...)       : R() );
    }

    bool empty() const noexcept {return (proxy == 0);}

    explicit operator bool () const noexcept {return !empty();}
    bool operator! () const noexcept {return empty();}

    void reset(const Closure& c = Closure()) {*this = c;}


private:
    Closure(void *self, proxy_m proxy) noexcept
    {
        this->self = self;
        this->proxy = (void*)proxy;
    }

    Closure(proxy_f proxy) noexcept
    {
        this->self = 0;
        this->proxy = (void*)proxy;
    }

    template <typename> friend struct closure::CreateHelper;
};


namespace closure
{
    template <typename... Args>
    struct ProxyFunc
    {
        template <typename T, typename R, R (T::*mem_func)(Args...)>
        static R Func(void* self, Args... args)
        {
            return (static_cast<T*>(self) ->* mem_func)(std::forward<Args>(args)...);
        }

        template <typename T, typename R, R (T::*mem_func)(Args...) const>
        static R Func(void* self, Args... args)
        {
            return (static_cast<T*>(self) ->* mem_func)(std::forward<Args>(args)...);
        }
    };

    // Обобщенная декларация
    template <typename> struct CreateHelper {};

    template <typename B, typename R, typename... Args>
    struct CreateHelper<R (B::*)(Args...)>
    {
        typedef Closure<R (Args...)> ClosureType;

        template <R (B::*mem_func)(Args...), typename D>
        static ClosureType Init(D *d) noexcept
        {
            // Проверяем корректность преобразования типа. Допускается преобразование
            // только от классов-наследников к базовым классам.
            B* b = d;
            return ClosureType(b, ProxyFunc<Args...>::template Func<B, R, mem_func>);
        }
    };

    template <typename B, typename R, typename... Args>
    struct CreateHelper<R (B::*)(Args...) const>
    {
        typedef Closure<R (Args...)> ClosureType;

        template <R (B::*mem_func)(Args...) const, typename D>
        static ClosureType Init(D *d) noexcept
        {
            B* b = d;
            return ClosureType(b, ProxyFunc<Args...>::template Func<B, R, mem_func>);
        }
    };

    template <typename R, typename... Args>
    struct CreateHelper<R (*)(Args...)>
    {
        typedef Closure<R (Args...)> ClosureType;

        template <R (*func)(Args...)>
        constexpr static ClosureType Init() noexcept
        {
            return ClosureType(func);
        }
    };

    template<typename Func, Func func, typename T>
    constexpr auto Create(T* t) -> typename CreateHelper<Func>::ClosureType
    {
        return CreateHelper<Func>::template Init<func>(t);
    }

    template<typename Func, Func func>
    constexpr auto Create() -> typename CreateHelper<Func>::ClosureType
    {
        return CreateHelper<Func>::template Init<func>();
    }

} // namespace closure


// Макрос для создания объекта Closure
// FUNC_PTR  Указатель на функцию член класса или на обычную функцию.
// OBJ_PTR   Указатель на экземпляр класса. В случае связывания обычной
//           функции этот параметр должен быть опущен.
//
#define CLOSURE(FUNC_PTR, OBJ_PTR...) (closure::Create<decltype(FUNC_PTR), FUNC_PTR>(OBJ_PTR))




