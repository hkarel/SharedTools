/****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализованы интеллектуальный указатель c внешним счетчиком
  ссылок.
****************************************************************************/

#pragma once

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif

#include "allocator_ptr.h"

#include <new>
#include <atomic>
#include <stdlib.h>
#include <type_traits>

/**
  Использование спец-аллокатора памяти для экземпляров counter_ptr_t
  дает небольшой проигрыш по сравнению с системным распределителем памяти.
*/
//#define USE_MEMMANAGER_CPTR
//#ifdef USE_MEMMANAGER_CPTR
//#include "_memalloc.h"
//#endif


#if defined(_MSC_VER)
extern "C" __declspec(selectany)
#endif
const bool fake_ptr = true;

#ifndef NDEBUG
#define GET_DEBUG _dbg = get();
#else
#define GET_DEBUG
#endif

#ifdef CONTAINER_PTR_DEBUG
#define PRINT_DEBUG(PRINT, VALUE) std::cout << (PRINT) << (VALUE) << "\n";
#else
#define PRINT_DEBUG(PRINT, VALUE)
#endif


/**
  @brief Структура для подсчета ссылок в container_ptr.
*/
struct counter_ptr_t
{
    std::atomic<uint32_t> count;

    void add_ref() {++count;}
    uint32_t release() {return --count;}

    // Флаг fake - определяет состояние ложного счетчика ссылок.
    // Если fake == true, то при уничтожении counter_ptr - объект владения
    // разрушен не будет, таким образом контейнер вырождается в обычный указатель.
    unsigned int fake : 1;

    // Флаг join - признак выделения единого сегмента памяти для counter_ptr_t
    // и целевого объекта. Основное назначение: сократить издежки на выделение/
    // освобождение памяти при работе с большим количеством маленьких однотипных
    // объектов.
    // !!! Не использовать с флагом fake == true - поведение не исследовано.
    // !!! Не использовать "внутри" спец-аллокаторов памяти - поведение не ис-
    //     следовано.
    // !!! Не использовать для объектов с полиморфным поведением: Base->Derived -
    //     поведение не исследовано.
    // [22.02.2013] Замечания по совместной работе fake и join: по сути эти пара-
    // метры являются взаимоисключающими. Смарт-указатель с флагом fake создается
    // уже для существующих объектов, и так, чтобы после разрушения смарт-указателя,
    // объект владения остался жив.
    // Типичный пример: есть некая функция some_func(container_ptr<T>), ее нужно
    // вызвать из метода класса и в качестве параметра отправить в нее this.
    // Если создать обычный (не fake) указатель, то при выходе из метода класса
    // получим убийство this. Поэтому единственный корректный способ выйти из
    // данной ситуации будет такой: some_func(container_ptr(this, fake_ptr)).
    // С другой стороны смарт-указатели с с флагом join всегда создаются с исполь-
    // зованием функции create_join_ptr(), что в принципе исключает использование
    // уже существующего адреса объекта владения.
    unsigned int join : 1;

    // Зарезервировано
    unsigned int reserved : 30;

    // Указатель на целевой объект. Используем тип void* чтобы не возникло проблем
    // с преобразованием типов.
    // Этот параметр должен идти последним в списке параметров, это позволит
    // экономить 4(8) байта при выделении единого сегмента памяти (см. описание join).
    void* ptr;

    void* __ptr() {return (join) ? &ptr : ptr;}
    //inline void* __ptr() const {if (join) {return (void*)&ptr;} return ptr;}

    // Конструктор
    //counter_ptr_t() : ptr(0), count(0), fake(0) {}
    //counter_ptr_t() {init();}
    //void init() {count = 1, fake = 0, join = 0, ptr = 0;}
    counter_ptr_t() NOEXCEPT
        : count(1), fake(0), join(0), reserved(0), ptr(0)
    {}

    ~counter_ptr_t() NOEXCEPT = default;

    counter_ptr_t(counter_ptr_t&&) = delete;
    counter_ptr_t(const counter_ptr_t&) = delete;

    counter_ptr_t& operator= (counter_ptr_t&&) = delete;
    counter_ptr_t& operator= (const counter_ptr_t&) = delete;
};


/**
  Вспомогательная структура, используется для определения наличия в аллокаторе
  функции разрушения вида: destroy(T* x, bool join). Эта функция используется
  для разрушения объектов созданных функцией container_ptr::create_join_ptr().
  Наличие (или отсутствие) функции destroy(T* x, bool join) определяет возможность
  создавать объекты с помощью функции container_ptr::create_join_ptr(), а так же
  определяет политику разрушения объектов. Политика разрушения объектов
  определяется структурами container_ptr_destroy<bool>.
*/
template<typename T, template<typename> class Allocator>
struct container_ptr_check_join
{
    static char  detect(void (*f)(T*, bool));
    static void* detect(...);
    enum {Yes = (1 == sizeof(detect(&Allocator<T>::destroy)))};
};


/**
  Вспомогательные структуры, определяют политики разрушения объекта в контейнере
  container_ptr в зависимости от наличия функции destroy(T* x, bool join)
  в управляющем аллокаторе.
  Общая политика позволяет разрушать объекты созданные с помощью оператора new,
  а так же созданные с помощью функции container_ptr::create_join_ptr().
  Для этого случая аллокатор должен иметь функцию разрушения вида:
  destroy(T* x, bool join).
  Специализированная политика позволяет разрушать объекты созданные с помощью
  оператора new (или других подобных операторов), но не допускает разрушения
  объектов созданных с помощью функции container_ptr::create_join_ptr().
*/
template<bool join_enable>
struct container_ptr_destroy
{
    template<typename T, template<typename> class Allocator>
    static void destroy(T* ptr, bool join) {Allocator<T>::destroy(ptr, join);}
};
template<>
struct container_ptr_destroy<false>
{
    template<typename T, template<typename> class Allocator>
    static void destroy(T* ptr, bool /*join*/) {Allocator<T>::destroy(ptr);}
};


/**
  @brief container_ptr - реализует интеллектуальный указатель с подсчетом
  ссылок.

  Важно: container_ptr не является потокобезопасным классом. Поэтому если допус-
  кается изменение container_ptr одновременно из нескольких потоков, то необхо-
  димо выполнять mutex-блокировки или atomic-блокировки при обращении к методам
  container_ptr.
  Примечание: если сделать все мотоды container_ptr потокобезопасными - это по-
  низит общую эффективность и быстродействие при использовании container_ptr.
*/
template<
    typename T,
    template<typename> class Allocator = allocator_ptr
>
class container_ptr
{
public:
    typedef T element_t;
    typedef Allocator<T>  allocator_t;
    typedef container_ptr<T, Allocator> self_t;

    //TODO: решить через container_ptr_traits
    //enum {IsContainerPtr = 1};

public:
    container_ptr() {
        // Не создаем счетчик для пустого контейнера, при создании большого
        // числа экземпляров container_ptr такое решение позволит меньше
        // фрагментировать память.
        //_counter = 0;
        GET_DEBUG
    }

    ~container_ptr() {
        PRINT_DEBUG("~container_ptr(), container_ptr_check_join: ", (container_ptr_check_join<T, Allocator>::Yes))
        __release(_counter);
    }

    explicit container_ptr(T* p, bool fake = false) {
        PRINT_DEBUG("explicit container_ptr(T* p, bool fake = false)", "")
        // Описание параметра fake см. в struct counter_ptr_t
        _counter = create_counter();
        //_counter->init();
        _counter->ptr = p;
        _counter->fake = fake;
        GET_DEBUG
    }

    // Дефолтные функции должны быть определены, иначе компилятор создаст их
    // неявно, и их поведение будет отличаться от ожидаемого.
    container_ptr(const self_t& p) {
        PRINT_DEBUG("container_ptr(const self_t&)", "")
        assign(p);
    }

    self_t& operator= (const self_t& p) {
        PRINT_DEBUG("operator= (const self_t&)", "")
        assign(p);
        return *this;
    }

    template<typename otherT, template<typename> class otherA>
    container_ptr(const container_ptr<otherT, otherA> & p) {
        PRINT_DEBUG("container_ptr(const container_ptr<otherT, otherA> &)", "")
        check_converting(p);
        assign(p);
    }

    template<typename otherT, template<typename> class otherA>
    self_t& operator= (const container_ptr<otherT, otherA> & p) {
        PRINT_DEBUG("operator= (const container_ptr<otherT, otherA> &)", "")
        check_converting(p);
        assign(p);
        return *this;
    }

    // Дефолтные функции должны быть определены, иначе компилятор создаст их
    // неявно, и их поведение будет отличаться от ожидаемого.
    container_ptr(self_t&& p) {
        PRINT_DEBUG("container_ptr(self_t&&)", "")
        assign(p, true);
    }

    self_t& operator= (self_t&& p) {
        PRINT_DEBUG("operator= (self_t&&)", "")
        assign(p, true);
        return *this;
    }

    template<typename otherT, template<typename> class otherA>
    container_ptr(container_ptr<otherT, otherA> && p) {
        PRINT_DEBUG("container_ptr(container_ptr<otherT, otherA> &&)", "")
        check_converting(p);
        assign(p, true);
    }

    template<typename otherT, template<typename> class otherA>
    self_t& operator= (container_ptr<otherT, otherA> && p) {
        PRINT_DEBUG("operator= (container_ptr<otherT, otherA> &&)", "")
        check_converting(p);
        assign(p, true);
        return *this;
    }


    // Динамическое преобразование типа.
    template<typename other_cptrT>
    other_cptrT dynamic_cast_to() const {
        if (!empty()) {
            typedef typename other_cptrT::element_t other_element_t;
            if (/*other_element_t* other_element = */dynamic_cast<other_element_t*>(get())) {
                other_cptrT other_cptr(_counter, 0, 0);
                return other_cptr;
            }
        }
        return other_cptrT();
    }

    // Проверяет возможность динамического преобразования к указанному типу.
    template<typename other_cptrT>
    bool dynamic_cast_to(int) const {
        if (!empty()) {
            typedef typename other_cptrT::element_t other_element_t;
            return dynamic_cast<other_element_t*>(get());
        }
        return false;
    }

    //self_t clone() const {
    //    STATIC_CHECK_CPTR(1, No_implement)
    //}

    T* get() const NOEXCEPT {return get(_counter);}

    T* operator-> () const NOEXCEPT {return  get();}
    T& operator*  () const NOEXCEPT {return *get();}
    operator T*   () const NOEXCEPT {return  get();}

    // Функция reset() введена вместо функции release(), это сделано
    // для того, чтобы осуществить однотипное поведение одноименных
    // функций в классах container_ptr и simple_ptr.
    // Примечание: присваиваться должен self_t() (без параметров), иначе будет
    //             дополнительный расход ресурсов на создание объекта counter_ptr_t.
    void reset() {assign(self_t(/*0*/));}

    bool empty() const NOEXCEPT {return (get() == 0);}

    explicit operator bool () const NOEXCEPT {return (get() != 0);}
    bool operator! () const NOEXCEPT {return (get() == 0);}

    // Функции совместимости с Qt
    T* data() const NOEXCEPT {return get();}
    bool isNull() const NOEXCEPT {return empty();}

    // Вспомогательные функции.
    static T* create() {return allocator_t::create();}
    static T* create(const T& x) {return allocator_t::create(&x);}
    static self_t create_ptr() {return self_t(allocator_t::create());}
    static self_t create_ptr(const T& x) {return self_t(allocator_t::create(&x));}

    // Создает объект container_ptr с единым сегментом памяти для целевого объекта
    // и экземпляра counter_ptr_t. После создания такого container_ptr необходимо вызвать
    // распределяющий оператор new() для целевого объекта следующим образом:
    //    container_ptr ptr = container_ptr<T>::create_join_ptr();
    //    new (ptr.get()) T();
    static self_t create_join_ptr() {
        enum {join_yes = container_ptr_check_join<T, Allocator>::Yes};
        static_assert(join_yes, "Allocators must have function with signature: destroy(T* x, bool join)");
        self_t self;
        const int size = sizeof(counter_ptr_t) - sizeof(((counter_ptr_t*)0)->ptr) + sizeof(T);
        void* ptr = malloc(size);
        if (ptr == 0)
            throw std::bad_alloc();
        self._counter = allocate_counter(ptr);
        self._counter->init();
        self._counter->join = true;
        return self;
    }

private:
    // Вспомогательный конструктор, используется в функции dynamic_cast_().
    // Добавлены два фиктивных параметра, чтобы избежать неоднозначностей компиляции.
    explicit container_ptr(/*const*/ counter_ptr_t* counter, int, int) {
        _counter = counter;
        if (_counter)
            _counter->add_ref();

        GET_DEBUG
    }

    static counter_ptr_t* create_counter() {
        void* ptr = malloc(sizeof(counter_ptr_t));
        if (ptr == 0)
            throw std::bad_alloc();
        return allocate_counter(ptr);
    }

    static counter_ptr_t* allocate_counter(void *ptr) {
        new (ptr) counter_ptr_t();
        return (counter_ptr_t*) ptr;
    }

    static void __release(counter_ptr_t* counter) {
        enum {join_yes = container_ptr_check_join<T, Allocator>::Yes};
        if (counter)
            if (counter->release() == 0) {
                if (counter->join) {
                    container_ptr_destroy<join_yes>::template destroy<T, Allocator>(get(counter), true);
                }
                else {
                    if (!counter->fake)
                        container_ptr_destroy<join_yes>::template destroy<T, Allocator>(get(counter), false);
                }
                counter->~counter_ptr_t();
                free(counter);
            }
    }

    static T* get(counter_ptr_t* counter) NOEXCEPT {
        return static_cast<T*>(counter ? counter->__ptr() : 0);
    }

    // Проверяет эквивалентность аллокаторов, и корректность преобразования
    // к базовому типу
    template<typename otherT, template<typename> class otherA>
    static void check_converting(const container_ptr<otherT, otherA> &) {
        // Проверка на эквивалентность аллокаторов
        static_assert(allocator_ptr_equal<Allocator, otherA>::Yes, "Allocators must be identical");

        // Проверяем корректность преобразования типа. Допускается преобразование
        // только от классов-наследников к базовым классам.
        // T* base = p.get();
        static_assert(std::is_base_of<T, otherT>::value, "Type otherT must be derived from T");
    }

//     template<typename otherT, template<typename> class otherA>
//     void assign(const container_ptr<otherT, otherA> & p, bool rvalue = false) {
//         // Потенциальная уязвимость: если эту функцию одновременно вызывать из
//         // нескольких потоков для одной и той же переменной, а это можно сделать
//         // только через оператор присваивания, то результат в _counter может
//         // быть неопределенным. Поэтому при присвоении необходимо использовать
//         // атомарные блокировки.
//         __release(_counter);
//         _counter = p._counter;
//         if (!rvalue) {
//             if (_counter)
//                 _counter->add_ref();
//         }
//         else {
//             const_cast<container_ptr<otherT, otherA> & >(p)._counter = 0;
//         }
//         GET_DEBUG
//     }

    // Потенциальная уязвимость функций assign: если эти функции одновременно
    // вызывать из нескольких потоков для одной и той же переменной, а это можно
    // сделать только через оператор присваивания, то результат в _counter может
    // быть неопределенным. Поэтому при присвоении необходимо использовать
    // атомарные блокировки.

    // Для использования в обычных операторах присваивания и копирования.
    template<typename otherT, template<typename> class otherA>
    void assign(const container_ptr<otherT, otherA> & p) {
        __release(_counter);
        _counter = p._counter;
        if (_counter)
            _counter->add_ref();
        GET_DEBUG
    }

    // Для использования в rvalue-операторах присваивания и копирования.
    template<typename otherT, template<typename> class otherA>
    void assign(container_ptr<otherT, otherA> & p, bool /*rvalue*/) {
        __release(_counter);
        _counter = p._counter;
        p._counter = 0;
        GET_DEBUG
    }

private:
    counter_ptr_t* _counter = {0};

#ifndef NDEBUG
    // Используется для просмотра в отладчике параметров типа Т,
    // counter_ptr_t - этого делать не позволяет, т.к. возвращает void*
    mutable T* _dbg;
#endif

    template<typename, template<typename> class> friend class container_ptr;
};


//template<typename CPtr> struct container_ptr_traits {enum{Yes = 0;}};
//template<typename CPtr> struct container_ptr_traits {enum{Yes = 0;}};


#undef GET_DEBUG
#undef PRINT_DEBUG

