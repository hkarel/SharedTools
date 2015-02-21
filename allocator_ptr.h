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
    static void destroy(T* x) {delete x;}

    // Функция используется при разрушении объектов хранимых в container_ptr,
    // join - признак использования единого сегмента памяти целевого объекта
    // и экземпляра counter_ptr_t.
    static void destroy(T* x, bool join) {
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
    static void destroy(T* x /*, bool*/) {delete [] x;}
};


/**
  @brief Используется для определения эквивалентных аллокаторов
  Примечание: реализация взяла из STL cpp_type_traits.h
*/
template<template<typename> class A1, template<typename> class A2 > struct allocator_ptr_equal
{
    enum {Yes = 0};
    //struct alloc_traits {char padding[1];};
    //struct other_alloc_traits {char padding[8];};
    //template<typename A>
    //alloc_traits is_same_allocator(const A&, const A&);
    //other_alloc_traits is_same_allocator(...);
};

template<template<typename> class A> struct allocator_ptr_equal<A, A>
{
    enum {Yes = 1};
};

//template<typename, typename> struct allocator_ptr_equal
//{
//    enum {value = 0};
//};
//template<typename T> struct allocator_ptr_equal<T, T>
//{
//    enum {value = 1};
//};

