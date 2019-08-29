/*****************************************************************************
  The MIT License

  Copyright © 2019 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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
#include <QHash>
#include <QSet>
#include <type_traits>

/**
  В Qt 4.8 классы QHash, QSet неправильно работают с некоторыми интегральными
  типами. Классы QHashEx, QSetEx исправляют эту ошибку.
*/
#if QT_VERSION >= 0x050000
template<typename Key, typename T> using QHashEx = QHash<Key, T>;
template<typename T> using QSetEx = QSet<T>;
#else
template<typename T>
struct QHashIntegral
{
    QHashIntegral() = default;
    QHashIntegral(T v) : value(v) {}
    operator T() const {return value;}
    T value = T{};
};

/* QHashEx */
template<typename Key, typename T, bool> struct QHashIntg;

template<typename Key, typename T>
struct QHashIntg<Key, T, false> : QHash<Key, T> {};

template<typename Key, typename T>
struct QHashIntg<Key, T, true> : QHash<QHashIntegral<Key>, T> {};

template<typename Key, typename T>
using QHashEx = QHashIntg<Key, T, std::is_integral<Key>::value>;

/* QSetEx */
template<typename T, bool> struct QSetIntg;

template<typename T>
struct QSetIntg<T, false> : QSet<T> {};

template<typename T>
struct QSetIntg<T, true> : QSet<QHashIntegral<T> > {};

template<typename T>
using QSetEx = QSetIntg<T, std::is_integral<T>::value>;
#endif

