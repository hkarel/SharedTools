/****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализованы инеллектуальный указатель simple_prt,
  аналог std::auto_ptr.
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
//#include <assert.h>


/**
  @brief simple_ptr - почти полный аналог auto_ptr и выполнен на его
  основе. Отличие состоит в том, что для simple_ptr определен аллокатор
  для создания и разрушения объекта владения.
*/
template<
    typename T,
    template<typename> class Allocator = allocator_ptr
>
class simple_ptr
{
public:
    typedef T  element_t;
    typedef Allocator<T>  allocator_t;
    typedef simple_ptr<T, Allocator>  self_t;

public:
    explicit simple_ptr(T* p = 0) : _ptr(p) {}
    ~simple_ptr() {allocator_t::destroy(_ptr);}

    // По аналогии с unique_ptr запрещаем конструктор копирования и
    // оператор присваивания.
    simple_ptr(self_t&) = delete;
    simple_ptr(const self_t&) = delete;
    //simple_ptr(self_t& p) {
    //    _ptr = 0;
    //    assign(p);
    //}

    self_t& operator= (self_t&) = delete;
    self_t& operator= (const self_t&) = delete;
    //self_t& operator= (self_t& p) {
    //    assign(p);
    //    return *this;
    //}

    simple_ptr(self_t&& p) {
        _ptr = 0;
        assign(p);
    }

    self_t& operator= (self_t&& p) {
        assign(p);
        return *this;
    }


    // Допускается использование копирующего конструктора для объектов, только
    // если аллокаторы идентичны.
    // Допускается запись вида:
    // simple_ptr<base-class> base_ptr;
    // simple_ptr<derived-class> derived_ptr;
    // base_ptr = derived_ptr - Верно,
    // simple_ptr<base-class> base_ptr2(derived_ptr) - Верно,
    // при условии, что для base-class и derived-class используется один и тот же
    // аллокатор.
    // Не допускается запись вида:
    // simple_ptr<int, allocator_ptr> p1;
    // simple_ptr<int, allocator_array_ptr> p2;
    // simple_ptr<int, allocator_array_ptr> p3(p1) - Ошибка
    // Так же не допускаются аналогичные преобразования для оператора operator= ()
    // p2 = p1 - Ошибка
    // p1 = p2 - Ошибка
    // (p1) p2 - Ошибка
    // (p2) p1 - Ошибка
    //
    // По аналогии с unique_ptr запрещаем конструктор копирования и
    // оператор присваивания.
    //
    //template<typename otherT, template<typename> class otherA>
    //simple_ptr(simple_ptr<otherT, otherA> & p) {
    //    _ptr = 0;
    //    assign(p);
    //}

    //// См. комментарии к simple_ptr(simple_ptr<otherT, Allocator> & sptr)
    //template<typename otherT, template<typename> class otherA>
    //self_t& operator= (simple_ptr<otherT, otherA> & p) {
    //    assign(p);
    //    return *this;
    //}

    template<typename otherT, template<typename> class otherA>
    simple_ptr(simple_ptr<otherT, otherA> && p) {
        _ptr = 0;
        assign(p);
    }

    template<typename otherT, template<typename> class otherA>
    self_t& operator= (simple_ptr<otherT, otherA> && p) {
        assign(p);
        return *this;
    }

    T* get() const NOEXCEPT {return _ptr;}

    T* operator-> () const NOEXCEPT {return  get();}
    T& operator*  () const NOEXCEPT {return *get();}
    operator T*   () const NOEXCEPT {return  get();}

    // Допускается использовать только для инициализации
    T** ref() {
        assert(_ptr == 0);
        return &_ptr;
    }

    T* release() NOEXCEPT {T* tmp(_ptr); _ptr = 0; return tmp;}

    // Функция введена для удобства, как замена конструкции
    // вида: simple_ptr<TYPE>(p.release()). Функция носит декоративный характер,
    // как правило используется для передачи параметра в функцию.
    // Запись через release_ptr() позволяет наглядно отразить тот факт, что
    // параметр передан в функцию и за ее пределами более не действителен.
    // Вызов просто p.release() может вступать в проиворечие с explicit-дирек-
    // тивой конструктора simple_ptr.
    self_t release_ptr() {return self_t(release());}

    void reset(T* p = 0) {
        if (_ptr != p)
            allocator_t::destroy(_ptr);
        _ptr = p;
    }

    bool empty() const NOEXCEPT {return (_ptr == 0);}

    explicit operator bool () const NOEXCEPT {return (_ptr != 0);}
    bool operator! () const NOEXCEPT {return (_ptr == 0);}

    // Функции совместимости с Qt
    T* data() const NOEXCEPT {return get();}
    T* take() NOEXCEPT {return release();}
    bool isNull() const NOEXCEPT {return empty();}

    // Размер элемента smart-pointer.
    int type_size() const NOEXCEPT {return sizeof(element_t);}

    // Вспомогательные функции.
    static T* create() {return allocator_t::create();}
    static T* create(const T& x) {return allocator_t::create(&x);}
    static self_t create_ptr() {return self_t(allocator_t::create());}
    static self_t create_ptr(const T& x) {return self_t(allocator_t::create(&x));}

private:
    template<typename otherT, template<typename> class otherA>
    void assign(simple_ptr<otherT, otherA> & p) {
        // Проверка на эквивалентность аллокаторов
        static_assert(allocator_ptr_equal<Allocator, otherA>::Yes, "Allocators must be identical");

        if (_ptr)
            allocator_t::destroy(_ptr);

        // Проверяем корректность преобразования типа, допускается
        // преобразование только к базовым типам.
        _ptr = p.release();
    }

private:
    T* _ptr = 0;
};

