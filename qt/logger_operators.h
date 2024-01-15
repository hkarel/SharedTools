/*****************************************************************************
  The MIT License

  Copyright © 2015 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  Модуль содержит специализированные потоковые операторы '<<' для логгера.

*****************************************************************************/

#pragma once

#include "logger/logger.h"
#include "logger/format.h"
#include "qt/quuidex.h"
#include <QtCore>
#ifdef QT_NETWORK_LIB
#include <QHostAddress>
#endif

namespace alog {

Line& operator<< (Line&, const QString&);
Line& operator<< (Line&, const QByteArray&);
Line& operator<< (Line&, const QUuid&);
Line& operator<< (Line&, const QTime&);
Line& operator<< (Line&, const QDate&);
Line& operator<< (Line&, const QDateTime&);
#ifdef QT_NETWORK_LIB
Line& operator<< (Line&, const QHostAddress&);
Line& operator<< (Line&, const QHostAddress::SpecialAddress);
#endif
Line& operator<< (Line&, const QVariant&);

template<int N>
Line& operator<< (Line& line, const QUuidT<N>& u)
{
    line << static_cast<const QUuid&>(u);
    return line;
}

namespace detail {

template<typename Array>
Line& printArray(Line& line, const Array& array)
{
    if (line.toLogger())
    {
        line << '[';
        bool first = true;
        for (auto it = array.begin(); it != array.end(); ++it)
        {
            const char* comma = ", ";
            if (first)
            {
                first = false;
                comma = nullptr;
            }
            line << comma << *it;
        }
        line << ']';
    }
    return line;
}

} // namespace detail

template<typename T, typename U>
Line& operator<< (Line& line, const QPair<T, U>& p)
{
    if (line.toLogger())
        line << '{' << p.first << ", " << p.second << '}';
    return line;
}

#if QT_VERSION < 0x060000
template<typename T>
Line& operator<< (Line& line, const QVector<T>& v)
{
    return detail::printArray(line, v);
}
#endif

template<typename T>
Line& operator<< (Line& line, const QList<T>& l)
{
    return detail::printArray(line, l);
}

template<typename T>
Line& operator<< (Line& line, const QSet<T>& s)
{
    return detail::printArray(line, s);
}

template<typename... Args>
inline Format<Args...> format(const QString& descript, Args&&... args)
{
    return Format<Args...>(descript.toUtf8().constData(), std::forward<Args>(args)...);
}

} // namespace alog
