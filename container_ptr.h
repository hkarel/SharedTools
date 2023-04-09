/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2010 Pavel Karelin (hkarel), <hkarel@yandex.ru>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  ---

  В модуле реализованы интеллектуальный указатель c внешним счетчиком ссылок
  
*****************************************************************************/

#pragma once

#include "allocator_ptr.h"

#include <new>
#include <atomic>
#include <utility>
#include <stdlib.h>
#include <type_traits>

/**
  Использование спец-аллокатора памяти для экземпляров counter_ptr_t дает
  небольшой проигрыш по сравнению с системным распределителем памяти
*/
//#define USE_MEMMANAGER_CPTR
//#ifdef USE_MEMMANAGER_CPTR
//#include "_memalloc.h"
//#endif

#if !defined(NDEBUG) || defined(DEBUGGING_ON_RELEASE)
#define GET_DEBUG  _dbg = get();
#define GET_DEBUG2 self._dbg = self.get();
#else
#define GET_DEBUG
#define GET_DEBUG2
#endif

#ifdef CONTAINER_PTR_DEBUG
#define PRINT_DEBUG(PRINT, VALUE) std::cout << (PRINT) << (VALUE) << "\n";
#else
#define PRINT_DEBUG(PRINT, VALUE)
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#pragma GCC diagnostic ignored "-Warray-bounds"

/**
  Вспомогательная структура, используется для подсчета ссылок в container_ptr
*/
struct counter_ptr_t
{
    std::atomic<uint32_t> count;

    void add_ref() {++count;}
    uint32_t release() {return --count;}

    // Флаг dummy определяет фиктивный счетчик ссылок. Если dummy равен true,
    // то при уничтожении counter_ptr - объект  владения  разрушен  не будет,
    // таким образом container_ptr вырождается в обычный указатель
    unsigned int dummy : 1;

    // Флаг join - признак выделения единого сегмента памяти для counter_ptr_t
    // и целевого объекта. Основное назначение: сократить издежки на выделение/
    // освобождение памяти при работе с большим количеством маленьких однотипных
    // объектов.
    // Не использовать join в следующих ситуациях (поведение не исследовано):
    //   1. Внутри спец-аллокаторов памяти;
    //   2. Для объектов с полиморфным поведением: Base->Derived.
    // [22.02.2013] Замечания  по совместной  работе  dummy и join: по сути эти
    // параметры  являются  взаимоисключающими.  container_ptr  с флагом  dummy
    // создается уже для существующих объектов, таким образом,  нельзя  создать
    // container_ptr одновременно с флагами join и dummy.
    // Типичный пример: есть некая функция some_func(container_ptr<T>), ее нужно
    // вызвать из метода класса, и в качестве параметра отправить в нее this.
    // Если создать обычный (не dummy) смарт-указатель, то при выходе из метода
    // класса получим разрушение  this.  Чтобы корректно выйти из этой ситуации
    // используется container_ptr с фиктивным счетчиком ссылок:
    //   some_func(container_ptr(this, true /*dummy*/)).
    // С другой стороны смарт-указатели  с флагом join всегда создаются через
    // функцию create_join(), что  в принципе,  исключает  использование  уже
    // существующего адреса объекта владения
    unsigned int join : 1;

    // Зарезервировано
    unsigned int reserved : 30;

    // Указатель на целевой объект.  Используем тип void*  чтобы не возникло
    // проблем с преобразованием типов.
    // Параметр ptr должен идти последним в списке параметров, это позволит
    // сэкономить 4/8 байт при выделении единого сегмента памяти
    // (см. описание join)
    void* ptr;

    void* __ptr() {return (join) ? &ptr : ptr;}

    counter_ptr_t() noexcept
        : count(1), dummy(false), join(false), reserved(0), ptr(nullptr)
    {}

    ~counter_ptr_t() noexcept = default;

    counter_ptr_t(counter_ptr_t&&) = delete;
    counter_ptr_t(const counter_ptr_t&) = delete;

    counter_ptr_t& operator= (counter_ptr_t&&) = delete;
    counter_ptr_t& operator= (const counter_ptr_t&) = delete;
};

/**
  Вспомогательная структура, используется для определения  наличия в аллокаторе
  функции разрушения вида:  destroy(T* x, bool join).  Эта функция используется
  для разрушения объектов созданных функцией container_ptr::create_join().
  Наличие (или отсутствие) функции destroy(T* x, bool join) определяет  возмож-
  ность  создавать  объекты  с  помощью  функции  container_ptr::create_join(),
  а так же определяет политику разрушения объектов. Политика разрушения объектов
  определяется структурами container_ptr_destroy<bool>
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
  container_ptr  в  зависимости  от  наличия  функции  destroy(T* x, bool join)
  в управляющем аллокаторе.
  Общая политика позволяет разрушать объекты созданные с помощью  оператора new,
  а так же созданные с помощью функции container_ptr::create_join().
  Для этого случая аллокатор должен иметь функцию разрушения вида:
    destroy(T* x, bool join).
  Специализированная политика позволяет разрушать  объекты  созданные с помощью
  оператора  new (или других подобных операторов),  но не допускает  разрушения
  объектов созданных с помощью функции container_ptr::create_join()
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
  Класс container_ptr - реализует интеллектуальный указатель с подсчетом ссылок.

  Важно: container_ptr не является потокобезопасным классом. Поэтому если допус-
  кается изменение container_ptr одновременно из нескольких потоков, то необхо-
  димо выполнять mutex-блокировки или atomic-блокировки при обращении к методам
  container_ptr.
  Примечание:  если сделать  все мотоды  container_ptr  потокобезопасными - это
  понизит общую эффективность и быстродействие при использовании  container_ptr
*/
template<
    typename T,
    template<typename> class Allocator = allocator_ptr
>
class container_ptr
{
public:
    typedef T element_t;
    typedef Allocator<T> allocator_t;
    typedef container_ptr<T, Allocator> self_t;

    enum {dummy_ptr = true};

public:
    container_ptr() noexcept {
        // Не создаем счетчик для пустого контейнера, при создании большого
        // числа экземпляров container_ptr такое  решение  позволит  меньше
        // фрагментировать память
        // _counter = nullptr;
        GET_DEBUG
    }

    container_ptr(std::nullptr_t) noexcept {
        GET_DEBUG
    }

    container_ptr(T* p, bool dummy /*см. counter_ptr_t::dummy*/) {
        PRINT_DEBUG("container_ptr(T* p, bool dummy). dummy = ", dummy)
        _counter = create_counter();
        _counter->ptr = p;
        _counter->join = false;
        _counter->dummy = dummy;
        GET_DEBUG
    }

    explicit container_ptr(T* p) : container_ptr(p, false /*dummy*/)
    {}

    ~container_ptr() {
        PRINT_DEBUG("~container_ptr(), container_ptr_check_join: ",
                    (container_ptr_check_join<T, Allocator>::Yes))
        release(_counter);
    }

    // Дефолтные функции должны быть определены, иначе компилятор создаст их
    // неявно, и их поведение будет отличаться от ожидаемого
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
    container_ptr(const container_ptr<otherT, otherA>& p) {
        PRINT_DEBUG("container_ptr(const container_ptr<otherT, otherA> &)", "")
        check_allocators_is_equal(p);
        check_converting_to_self_type(p);
        assign(p);
    }

    template<typename otherT, template<typename> class otherA>
    self_t& operator= (const container_ptr<otherT, otherA>& p) {
        PRINT_DEBUG("operator= (const container_ptr<otherT, otherA> &)", "")
        check_allocators_is_equal(p);
        check_converting_to_self_type(p);
        assign(p);
        return *this;
    }

    // Дефолтные функции должны быть определены, иначе компилятор создаст их
    // неявно, и их поведение будет отличаться от ожидаемого
    container_ptr(self_t&& p) {
        PRINT_DEBUG("container_ptr(self_t&&)", "")
        assign_rvalue(p);
    }

    self_t& operator= (self_t&& p) {
        PRINT_DEBUG("operator= (self_t&&)", "")
        assign_rvalue(p);
        return *this;
    }

    template<typename otherT, template<typename> class otherA>
    container_ptr(container_ptr<otherT, otherA>&& p) {
        PRINT_DEBUG("container_ptr(container_ptr<otherT, otherA> &&)", "")
        check_allocators_is_equal(p);
        check_converting_to_self_type(p);
        assign_rvalue(p);
    }

    template<typename otherT, template<typename> class otherA>
    self_t& operator= (container_ptr<otherT, otherA>&& p) {
        PRINT_DEBUG("operator= (container_ptr<otherT, otherA> &&)", "")
        check_allocators_is_equal(p);
        check_converting_to_self_type(p);
        assign_rvalue(p);
        return *this;
    }

    // Проверяет возможность динамического преобразования к указанному типу
    template<typename other_cptrT>
    bool dynamic_cast_is_possible() const {
        other_cptrT p;
        check_allocators_is_equal(p);
        if (!empty()) {
            typedef typename other_cptrT::element_t other_element_t;
            return dynamic_cast<other_element_t*>(get());
        }
        return false;
    }

    // Динамическое преобразование типа
    template<typename other_cptrT>
    other_cptrT dynamic_cast_to() const {
        return (dynamic_cast_is_possible<other_cptrT>())
               ? other_cptrT(_counter, 0, 0)
               : other_cptrT();
    }

    //self_t clone() const {
    //    STATIC_CHECK_CPTR(1, No_implement)
    //}

    T* get() const noexcept {return get(_counter);}

    T* operator-> () const noexcept {return  get();}
    T& operator*  () const noexcept {return *get();}
    operator T*   () const noexcept {return  get();}

    // Функция reset() введена вместо функции release(),  это сделано для того,
    // чтобы осуществить однотипное поведение  одноименных  функций  в классах
    // container_ptr и simple_ptr.
    // Примечание: параметр self_t()  должен быть  без параметров, иначе будут
    //             дополнительные  затраты  на создание  объекта counter_ptr_t
    void reset() {assign(self_t(/*0*/));}

    bool empty() const noexcept {return (get() == nullptr);}

    explicit operator bool () const noexcept {return !empty();}
    bool operator! () const noexcept {return empty();}

    // Функции совместимости с Qt
    [[deprecated]] T* data() const noexcept {return get();}
    [[deprecated]] bool isNull() const noexcept {return empty();}

    // Вспомогательные функции
    static self_t create() {return self_t(allocator_t::create());}
    static self_t create(const T& x) {return self_t(allocator_t::create(&x));}

    [[deprecated]] static self_t create_ptr() {return self_t(allocator_t::create());}
    [[deprecated]] static self_t create_ptr(const T& x) {return self_t(allocator_t::create(&x));}

    // Создает объект container_ptr с единым сегментом памяти для целевого
    // объекта и экземпляра counter_ptr_t
    template<typename... Args>
    static self_t create_join(Args&&... args) {
        enum {join_yes = container_ptr_check_join<T, Allocator>::Yes};
        static_assert(join_yes, "Allocator must have function "
                                "with signature: destroy(T* x, bool join)");
        self_t self;
        const int size = sizeof(counter_ptr_t)
                         - sizeof(((counter_ptr_t*)0)->ptr) + sizeof(T);
        void* ptr = malloc(size);
        if (ptr == nullptr)
            throw std::bad_alloc();
        self._counter = allocate_counter(ptr);
        self._counter->join = true;
        self._counter->dummy = false;
        new (get(self._counter)) T(std::forward<Args>(args)...);
        GET_DEBUG2
        return self;
    }

    template<typename... Args>
    [[deprecated]] static self_t create_join_ptr(Args&&... args) {
        return create_join(std::forward<Args>(args)...);
    }

private:
    // Вспомогательный конструктор, используется в функции dynamic_cast_to().
    // Добавлены два фиктивных параметра,  чтобы  избежать  неоднозначностей
    // подстановки при компиляции
    container_ptr(/*const*/ counter_ptr_t* counter, int, int) {
        _counter = counter;
        if (_counter)
            _counter->add_ref();
        GET_DEBUG
    }

#pragma GCC diagnostic push
#if __GNUC__ > 10
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

    static counter_ptr_t* create_counter() {
        void* ptr = malloc(sizeof(counter_ptr_t));
        if (ptr == nullptr)
            throw std::bad_alloc();
        return allocate_counter(ptr);
    }

    static counter_ptr_t* allocate_counter(void* ptr) {
        new (ptr) counter_ptr_t();
        return (counter_ptr_t*) ptr;
    }

    static void release(counter_ptr_t* counter) {
        enum {join_yes = container_ptr_check_join<T, Allocator>::Yes};
        if (counter)
            if (counter->release() == 0) {
                if (counter->join) {
                    container_ptr_destroy<join_yes>::template
                        destroy<T, Allocator>(get(counter), true);
                }
                else {
                    if (!counter->dummy)
                        container_ptr_destroy<join_yes>::template
                            destroy<T, Allocator>(get(counter), false);
                }
                counter->~counter_ptr_t();
                free(counter);
            }
    }

#pragma GCC diagnostic pop

    static T* get(counter_ptr_t* counter) noexcept {
        return static_cast<T*>(counter ? counter->__ptr() : nullptr);
    }

    // Проверяет эквивалентность аллокаторов
    template<typename otherT, template<typename> class otherA>
    static void check_allocators_is_equal(
                                      const container_ptr<otherT, otherA>&) {
        static_assert(allocator_ptr_equal<Allocator, otherA>::Yes,
                      "Allocators must be identical");
    }

    // Проверяет корректность преобразования типа otherT к типу T
    template<typename otherT, template<typename> class otherA>
    static void check_converting_to_self_type(
                                      const container_ptr<otherT, otherA>&) {
        static_assert(std::is_base_of<T, otherT>::value,
                      "Type otherT must be derived from T");
    }

    // Потенциальная уязвимость функций assign()/assign_rvalue(): если функция
    // одновременно вызывается из разных  потоков  для одной и той же перемен-
    // ной, что возможно сделать только через оператор присваивания, результат
    // в _counter может быть неопределенным. Поэтому, при присваивании смарт-
    // переменной значений из разных потоков необходимо использовать атомарные
    // блокировки

    // Для использования в обычных операторах присваивания и копирования
    template<typename otherT, template<typename> class otherA>
    void assign(const container_ptr<otherT, otherA>& p) {
        release(_counter);
        _counter = p._counter;
        if (_counter)
            _counter->add_ref();
        GET_DEBUG
    }

    // Для использования в rvalue-операторах присваивания и копирования
    template<typename otherT, template<typename> class otherA>
    void assign_rvalue(container_ptr<otherT, otherA>& p) {
        release(_counter);
        _counter = p._counter;
        p._counter = nullptr;
        GET_DEBUG
    }

private:
    counter_ptr_t* _counter = {nullptr};

#if !defined(NDEBUG) || defined(DEBUGGING_ON_RELEASE)
    // Используется для просмотра в отладчике параметров типа Т,
    // counter_ptr_t этого делать не позволяет, так как возвращает void*
    mutable T* _dbg = {nullptr};
#endif

    template<typename, template<typename> class> friend class container_ptr;
};

// GCC diagnostic ignored "-Wfree-nonheap-object"
#pragma GCC diagnostic pop

#undef GET_DEBUG
#undef GET_DEBUG2
#undef PRINT_DEBUG
