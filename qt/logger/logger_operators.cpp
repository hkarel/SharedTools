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

Line& operator<< (Line& line, const QHostAddress& h)
{
    if (line.toLogger())
        line << QStringToUtf8(h.toString());
    return line;
}

Line operator<< (Line&& line, const QHostAddress& h)
{
    if (line.toLogger())
        line << QStringToUtf8(h.toString());
    return std::move(line);
}


} // namespace alog
