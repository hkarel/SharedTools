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
*****************************************************************************/

#include "qt/logger/logger_operators.h"

namespace alog {

#if QT_VERSION >= 0x050000
inline std::string QStringToUtf8(const QString& s) {return std::move(s.toStdString());}
inline std::string QByteArrayToUtf8(const QByteArray& b) {return std::move(b.toStdString());}
#else
inline std::string QStringToUtf8(const QString& s) {return s.toUtf8().constData();}
inline std::string QByteArrayToUtf8(const QByteArray& b) {return b.constData();}
#endif

Line& operator<< (Line& line, const QString& s)
{
    if (line.toLogger())
        line << QStringToUtf8(s);
    return line;
}

Line operator<< (Line&& line, const QString& s)
{
    if (line.toLogger())
        line << QStringToUtf8(s);
    return std::move(line);
}

Line& operator<< (Line& line, const QByteArray& b)
{
    if (line.toLogger())
        line << QByteArrayToUtf8(b);
    return line;
}

Line operator<< (Line&& line, const QByteArray& b)
{
    if (line.toLogger())
        line << QByteArrayToUtf8(b);
    return std::move(line);
}

Line& operator<< (Line& line, const QUuid& u)
{
    if (line.toLogger())
        line << QStringToUtf8(u.toString());
    return line;
}

Line operator<< (Line&& line, const QUuid& u)
{
    if (line.toLogger())
        line << QStringToUtf8(u.toString());
    return std::move(line);
}

Line& operator<< (Line& line, const QUuidEx& u)
{
    if (line.toLogger())
        line << QStringToUtf8(u.toString());
    return line;
}

Line operator<< (Line&& line, const QUuidEx& u)
{
    if (line.toLogger())
        line << QStringToUtf8(u.toString());
    return std::move(line);
}

#ifdef QT_NETWORK_LIB
Line& operator<< (Line& line, const QHostAddress& h)
{
    if (line.toLogger())
        line << QStringToUtf8(h.isNull() ? QString("undefined") : h.toString());
    return line;
}

Line operator<< (Line&& line, const QHostAddress& h)
{
    if (line.toLogger())
        line << QStringToUtf8(h.isNull() ? QString("undefined") : h.toString());
    return std::move(line);
}
#endif

} // namespace alog
