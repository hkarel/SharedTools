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

#include <QMetaType>
#include <QUuid>
#include <QPair>
#include <QString>

/**
  Класс QUuidT полностью повторяет функционал QUuid, за исключением  переопреде-
  ленных операторов равенства, неравенства, и добавленного оператора  сравнения.
  Класс сделан шаблонным для того чтобы можно было "обмануть" макрос регистрации
  Q_DECLARE_METATYPE для типа QVariant.

  Если клас будет нешаблонным, то невозможно будет сделать следующую запись:
  typedef QUuidT IndexId;
  typedef QUuidT NodeId;
  Q_DECLARE_METATYPE(IndexId)
  Q_DECLARE_METATYPE(NodeId) // здесь будет ошибка повторного определения

  Для шаблонного класса проблем не возникнет:
  typedef QUuidT<1> IndexId;
  typedef QUuidT<2> NodeId;
  Q_DECLARE_METATYPE(IndexId)
  Q_DECLARE_METATYPE(NodeId) // ок!
*/
template<int N> struct QUuidT : public QUuid
{
public:
    constexpr QUuidT() noexcept : QUuid() {}
    constexpr QUuidT(const QUuid&  u) noexcept : QUuid(u) {}
    constexpr QUuidT(const QUuidT& u) noexcept : QUuid(u) {}
    constexpr QUuidT(uint l, ushort w1, ushort w2,
                     uchar b1, uchar b2, uchar b3, uchar b4,
                     uchar b5, uchar b6, uchar b7, uchar b8) noexcept
        : QUuid(l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
    {}
    explicit QUuidT(const char* uuidStr) noexcept : QUuid(QByteArray(uuidStr))
    {}
    explicit QUuidT(const QString& uuidStr) noexcept : QUuid(uuidStr)
    {}
    explicit QUuidT(const QByteArray& uuidStr) noexcept : QUuid(uuidStr)
    {}
    explicit QUuidT(quint64 ui1, quint64 ui2) noexcept
    {
        ((quint64*) this)[0] = ui1;
        ((quint64*) this)[1] = ui2;
    }
    QUuidT& operator= (const QUuid &u) noexcept
    {
        QUuid::operator= (u);
        return *this;
    }

#define COMPARE(f1, f2) if (f1 != f2) return (f1 < f2) ? -1 : 1;

    // compare
    template<typename UuidT1, typename UuidT2>
    static inline int compare(const UuidT1& u1, const UuidT2& u2) noexcept
    {
        static_assert(sizeof(u1) == 16, "Parameter u1 should be size 16 byte");
        static_assert(sizeof(u2) == 16, "Parameter u2 should be size 16 byte");
#if defined(__SIZEOF_INT128__)
        COMPARE(*((__uint128_t*) &u1), *((__uint128_t*) &u2))
#elif defined(__x86_64__)
        COMPARE(((quint64*) &u1)[0], ((quint64*) &u2)[0])
        COMPARE(((quint64*) &u1)[1], ((quint64*) &u2)[1])
#else
        COMPARE(((quint32*) &u1)[0], ((quint32*) &u2)[0])
        COMPARE(((quint32*) &u1)[1], ((quint32*) &u2)[1])
        COMPARE(((quint32*) &u1)[2], ((quint32*) &u2)[2])
        COMPARE(((quint32*) &u1)[3], ((quint32*) &u2)[3])
#endif
        return 0;
    }

#undef COMPARE

#if defined(__SIZEOF_INT128__)
    bool isNull() const noexcept {return (*((__uint128_t*) this) == 0);}
#elif defined(__x86_64__)
    bool isNull() const noexcept
    {
        return (((quint64*) this)[0] == 0) && (((quint64*) this)[1] == 0);
    }
#endif

    // equal
    template<typename UuidT1, typename UuidT2>
    static inline bool equal(const UuidT1& u1, const UuidT2& u2) noexcept
    {
        return (compare(u1, u2) == 0);
    }

    template<int M>
    bool operator== (const QUuidT<M>& u) const noexcept {return equal(*this, u);}
    template<int M>
    bool operator!= (const QUuidT<M>& u) const noexcept {return !equal(*this, u);}

    bool operator== (const QUuid& u) const noexcept {return  equal(*this, u);}
    bool operator!= (const QUuid& u) const noexcept {return !equal(*this, u);}

    template<int M>
    bool operator< (const QUuidT<M>& u) const noexcept {return (compare(*this, u) < 0);}
    template<int M>
    bool operator> (const QUuidT<M>& u) const noexcept {return (compare(*this, u) > 0);}

    bool operator< (const QUuid& u) const noexcept {return (compare(*this, u) < 0);}
    bool operator> (const QUuid& u) const noexcept {return (compare(*this, u) > 0);}
};

template<int N>
inline bool operator== (const QUuid& u1, const QUuidT<N>& u2) noexcept
{
    return QUuidT<N>::equal(u1, u2);
}

template<int N>
inline bool operator!= (const QUuid& u1, const QUuidT<N>& u2) noexcept
{
    return !QUuidT<N>::equal(u1, u2);
}

template<int N>
inline QDataStream& operator<< (QDataStream& s, const QUuidT<N>& u)
{
    return operator<< (s, static_cast<const QUuid&>(u));
}

template<int N>
inline QDataStream& operator>> (QDataStream& s, QUuidT<N>& u)
{
    return operator>> (s, static_cast<QUuid&>(u));
}

/**
  Сервисные функции, создают 64-битный хеш из QUuid
*/
inline quint64 hash64(const QUuid& u)
{
    static_assert(sizeof(u) == 16, "Parameter 'u' should be size 16 byte");
    //quint64 p1 = ((quint64*) &u)[0];
    //quint64 p2 = ((quint64*) &u)[1];
    //return (p1 ^ p2);
    return (((quint64*) &u)[0] ^ ((quint64*) &u)[1]);
}

template<int N>
inline quint64 hash64(const QUuidT<N>& u)
{
    return hash64(static_cast<const QUuid&>(u));
}

/**
  Дайжест-функции, возвращают первые 8 символов строкового представления uuid-а
*/
inline QString digest(const QUuid& u)
{
    return u.toString().mid(1, 8);
}

template<int N>
inline QString digest(const QUuidT<N>& u)
{
    return digest(static_cast<const QUuid&>(u));
}

/**
  Сервисные функции, возвращают строковое представление uuid без обрамляющих
  фигурных скобок
*/
inline QString toString(const QUuid& u)
{
    return u.toString(QUuid::StringFormat::WithoutBraces);
}

template<int N>
inline QString toString(const QUuidT<N>& u)
{
    return toString(static_cast<const QUuid&>(u));
}

/**
  Сервисные функции, возвращают пару значений UInt64
*/
inline QPair<quint64, quint64> toUIntUInt64(const QUuid& u)
{
    return {((quint64*) &u)[0], ((quint64*) &u)[1]};
}

template<int N>
inline QPair<quint64, quint64> toUIntUInt64(const QUuidT<N>& u)
{
    return toUIntUInt64(static_cast<const QUuid&>(u));
}

typedef QUuidT<0> QUuidEx;
Q_DECLARE_METATYPE(QUuidEx)

#if QT_VERSION < 0x050000
Q_DECLARE_METATYPE(QUuid)
#endif
