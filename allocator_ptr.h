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

  В модуле реализованы аллокаторы для инеллектуальных указатель simple_prt и
  container_ptr
*****************************************************************************/

#pragma once

/**
  Типовой распределитель памяти для еденичного объекта.
  Распределитель памяти не может иметь состояний  и все его  члены  должны
  быть статическими. В противном случае возможны конфликты для конструкций
  вида: container_ptr<base-class>(new derived-class)
*/
template<typename T> struct allocator_ptr
{
    // Функция создания объектов.
    // Примечание: должно быть две функции create(). Если использовать только
    // одну функцию вида:
    //   T* create(const T* x = 0) {return (x) ? new T(*x) : new T();}
    // то  компилятор  будет  требовать  обязательное  наличие  конструктора
    // копирования у инстанциируемого класса
    static T* create() {return new T();}
    static T* create(const T* x) {return (x) ? new T(*x) : new T();}

    // Функция разрушения объектов
    static void destroy(T* x) {
        static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
        delete x;
    }

    // Функция используется при разрушении объектов хранимых в container_ptr,
    // join - признак использования единого сегмента памяти целевого объекта
    // и экземпляра counter_ptr_t
    static void destroy(T* x, bool join) {
        static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
        if (x && join) x->~T(); else delete x;
    }
};

/**
  Типовой распределитель памяти для массива объектов
*/
template<typename T> struct allocator_array_ptr
{
    // Функция разрушения объектов
    static void destroy(T* x) {
        static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
        delete [] x;
    }

    // Примечание: не используем array-аллокаторы для экземпляров container_ptr
    // созданных при помощи  функции  create_join_ptr(), так как  в этом случае
    // невозможно корректно освободить память
    // static void destroy(T* x, bool join)
};

/**
  Используется для определения эквивалентных аллокаторов.
  Примечание: реализация взята из STL cpp_type_traits.h
*/
template<template<typename> class A1,
         template<typename> class A2 > struct allocator_ptr_equal
{
    enum {Yes = 0};
};

template<template<typename> class A> struct allocator_ptr_equal<A, A>
{
    enum {Yes = 1};
};
