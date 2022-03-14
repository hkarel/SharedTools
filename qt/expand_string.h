/*****************************************************************************
  The MIT License

  Copyright © 2022 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "quuidex.h"
#include <QtCore>

inline void expandString(QString&) {}

template<typename T, typename... Args>
void expandString(QString& descript, const T t, const Args&... args)
{
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value,
                  "The T must be integral type");

    descript = descript.arg(t);
    expandString(descript, args...);
}

template<typename... Args>
void expandString(QString& descript, const char* t, const Args&... args)
{
    descript = descript.arg(QString::fromUtf8(t));
    expandString(descript, args...);
}

template<typename... Args>
void expandString(QString& descript, const QString& s, const Args&... args)
{
    descript = descript.arg(s);
    expandString(descript, args...);
}

template<typename... Args>
void expandString(QString& descript, const QUuidEx& u, const Args&... args)
{
    descript = descript.arg(u.toString(QUuid::StringFormat::WithoutBraces));
    expandString(descript, args...);
}

template<typename... Args>
void expandString(QString& descript, const QDateTime& dt, const Args&... args)
{
    descript = descript.arg(dt.toString("dd.MM.yyyy hh:mm:ss.zzz"));
    expandString(descript, args...);
}

template<typename... Args>
void expandString(QString& descript, const QDate& d, const Args&... args)
{
    descript = descript.arg(d.toString("dd.MM.yyyy"));
    expandString(descript, args...);
}
