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
*****************************************************************************/

#pragma once

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif

#include "allocator_ptr.h"
#include <cassert>

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
    explicit simple_ptr(T* p = nullptr) : _ptr(p) {}
    ~simple_ptr() {allocator_t::destroy(_ptr);}

    // По аналогии с unique_ptr запрещаем конструктор копирования и
    // оператор присваивания.
    simple_ptr(self_t&) = delete;
    simple_ptr(const self_t&) = delete;

    self_t& operator= (self_t&) = delete;
    self_t& operator= (const self_t&) = delete;

    simple_ptr(self_t&& p) {
        _ptr = nullptr;
        assign(p);
    }

    self_t& operator= (self_t&& p) {
        assign(p);
        return *this;
    }

    template<typename otherT, template<typename> class otherA>
    simple_ptr(simple_ptr<otherT, otherA> && p) {
        _ptr = nullptr;
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
        assert(_ptr == nullptr);
        return &_ptr;
    }

    T* release() NOEXCEPT {T* tmp {_ptr}; _ptr = nullptr; return tmp;}

    // Функция введена для удобства, как замена конструкции
    // вида: simple_ptr<TYPE>(p.release()). Функция носит декоративный характер,
    // как правило используется для передачи параметра в функцию.
    // Запись через release_ptr() позволяет наглядно отразить тот факт, что
    // параметр передан в функцию и за ее пределами более не действителен.
    // Вызов просто p.release() может вступать в проиворечие с explicit-дирек-
    // тивой конструктора simple_ptr.
    self_t release_ptr() {return self_t(release());}

    void reset(T* p = nullptr) {
        if (_ptr != p)
            allocator_t::destroy(_ptr);
        _ptr = p;
    }

    bool empty() const NOEXCEPT {return (_ptr == nullptr);}

    explicit operator bool () const NOEXCEPT {return !empty();}
    bool operator! () const NOEXCEPT {return empty();}

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
    T* _ptr = {nullptr};
};

