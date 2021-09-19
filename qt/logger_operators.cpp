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

#include "qt/logger_operators.h"

namespace alog {

#if QT_VERSION >= 0x050000
inline std::string QStringToUtf8(const QString& s) {return s.toStdString();}
inline std::string QByteArrayToUtf8(const QByteArray& b) {return b.toStdString();}
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

Line& operator<< (Line& line, const QByteArray& b)
{
    if (line.toLogger())
        line << QByteArrayToUtf8(b);
    return line;
}

Line& operator<< (Line& line, const QUuid& u)
{
    if (line.toLogger())
        line << u.toString(QUuid::StringFormat::WithoutBraces);
    return line;
}

Line& operator<< (Line& line, const QTime& t)
{
    if (line.toLogger())
        line << t.toString("hh:mm:ss.zzz");
    return line;
}

Line& operator<< (Line& line, const QDate& d)
{
    if (line.toLogger())
        line << d.toString("dd.MM.yyyy");
    return line;
}

Line& operator<< (Line& line, const QDateTime& dt)
{
    if (line.toLogger())
        line << dt.toString("dd.MM.yyyy hh:mm:ss.zzz");
    return line;
}

#ifdef QT_NETWORK_LIB
Line& operator<< (Line& line, const QHostAddress& h)
{
    if (line.toLogger())
        line << QStringToUtf8(h.isNull() ? QString("undefined") : h.toString());
    return line;
}
#endif

template<typename List>
void printList(Line& line, const List& list)
{
    line << "[";
    for (int i = 0; i < list.count(); ++i)
    {
        if (i != 0)
            line << ", ";
        line << list[i];
    }
    line << "]";
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

Line& operator<< (Line& line, const QVariant& v)
{
    if (!line.toLogger())
        return line;

    if (v.type() == QVariant::Type(qMetaTypeId<float>()))
    {
        line << v.toFloat();
        return line;
    }

    if (v.isNull())
    {
        line << "QVariant is null";
        return line;
    }

    switch (v.type())
    {
        case QVariant::Invalid:
            line << "Invalid QVariant";
            break;

        case QVariant::Bool:
            line << v.toBool();
            break;

        case QVariant::ByteArray:
            line << v.toByteArray();
            break;

        case QVariant::String:
        {
            QString s = v.toString();
            if (s.isEmpty())
                s = "\"\"";
            line << s;
            break;
        }
        case QVariant::Int:
            line << v.toInt();
            break;

        case QVariant::UInt:
            line << v.toUInt();
            break;

        case QVariant::LongLong:
            line << v.toLongLong();
            break;

        case QVariant::ULongLong:
            line << v.toULongLong();
            break;

        case QVariant::Double:
            line << v.toDouble();
            break;

        case QVariant::Time:
        {
            QTime t = v.toTime();
            if (t.isValid())
                line << t.toString(Qt::ISODate);
            else
                line << "Invalid QTime";
            break;
        }
        case QVariant::Date:
        {
            QDate d = v.toDate();
            if (d.isValid())
                line << d.toString(Qt::ISODate);
            else
                line << "Invalid QDate";
            break;
        }
        case QVariant::DateTime:
        {
            QDateTime dt = v.toDateTime();
            if (dt.isValid())
                line << dt.toString(Qt::ISODate);
            else
                line << "Invalid QDateTime";
            break;
        }
#if QT_VERSION >= 0x050000
        case QVariant::Uuid:
        {
            const QUuid& u = v.toUuid();
            line << u;
            break;
        }
#endif
        case QVariant::UserType:
        {
            if (v.userType() == qMetaTypeId<QUuidEx>())
            {
                const QUuidEx& u = v.value<QUuidEx>();
                line << u;
            }
#if QT_VERSION < 0x050000
            else if (v.userType() == qMetaTypeId<QUuid>())
            {
                const QUuid& u = v.value<QUuid>();
                line << u;
            }
#endif
            else if (v.userType() == qMetaTypeId<QVector<QUuid>>())
            {
                const QVector<QUuid>& vect = v.value<QVector<QUuid>>();
                printList(line, vect);
            }
            else if (v.userType() == qMetaTypeId<QVector<QUuidEx>>())
            {
                const QVector<QUuidEx>& vect = v.value<QVector<QUuidEx>>();
                printList(line, vect);
            }
            else if (v.userType() == qMetaTypeId<QVector<qint32>>())
            {
                const QVector<qint32>& vect = v.value<QVector<qint32>>();
                printList(line, vect);
            }
            else
            {
                line << "Unsupported QVariant user-type for logger"
                     << "; Type name: " << v.typeName();
            }
            break;
        }
        default:
            line << "Unsupported QVariant type for logger"
                 << "; Type name: " << v.typeName();
    }
    return line;
}

#pragma GCC diagnostic pop

} // namespace alog
