/* clang-format off */
/****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализованы аллокаторы для инеллектуальных указатель simple_prt и
  container_ptr.
****************************************************************************/

#pragma once


/**
  @brief Типовой распределитель памяти для еденичного объекта.
  Распределитель памяти не может иметь состояний и все его члены должны
  быть статическими. В противном случае возможны конфликты для конструкций
  вида: container_ptr<base-class>(new derived-class).
*/
template<typename T> struct allocator_ptr
{
    // Функция создания объектов.
    // Примечание: должно быть две функции create(). Если использовать только
    // одну функцию вида T* create(const T* x = 0){return (x) ? new T(*x) : new T();}
    // то компилятор будет требовать обязательное наличие конструктора копирования
    // у инстанциируемого класса.
    static T* create() {return new T();}
    static T* create(const T* x) {return (x) ? new T(*x) : new T();}

    // Функция разрушения объектов.
    static void destroy(T* x) {
        static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
        delete x;
    }

    // Функция используется при разрушении объектов хранимых в container_ptr,
    // join - признак использования единого сегмента памяти целевого объекта
    // и экземпляра counter_ptr_t.
    static void destroy(T* x, bool join) {
        static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
        if (x) {if (join) x->~T(); else delete x;}
    }
};


/**
  @brief Типовой распределитель памяти для массива объектов.
  Примечание: не использовать array-аллокаторы для container_ptr при условии,
  что container_ptr будет создаваться с использованием функции create_join_ptr().
*/
template<typename T> struct allocator_array_ptr
{
    // Функция разрушения объектов
    // Второй параметр введен для совместимости с аллокаторами container_ptr.
    static void destroy(T* x /*, bool*/) {
        static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
        delete [] x;
    }
};


/**
  @brief Используется для определения эквивалентных аллокаторов
  Примечание: реализация взята из STL cpp_type_traits.h
*/
template<template<typename> class A1, template<typename> class A2 > struct allocator_ptr_equal
{
    enum {Yes = 0};
};

template<template<typename> class A> struct allocator_ptr_equal<A, A>
{
    enum {Yes = 1};
};
