/*****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализован класс QUuidT. Он полностью повторяет функционал QUuid,
  за исключением переопределенных операторов равенства и неравенства, и добав-
  ленного оператора сравнения.
  Класс сделан шаблонным для того чтобы можно было "обмануть" макрос
  регистрации Q_DECLARE_METATYPE для типа QVariant.
  Если клас будет нешаблонным, то невозможно будет сделать следующую
  запись:
  typedef QUuidT IndexId;
  typedef QUuidT NodeId;
  Q_DECLARE_METATYPE(IndexId)
  Q_DECLARE_METATYPE(NodeId) // здесь будет ошибка повторного определения

  А для шаблонного класса проблем не возникнет:
  typedef QUuidT<1> IndexId;
  typedef QUuidT<2> NodeId;
  Q_DECLARE_METATYPE(IndexId)
  Q_DECLARE_METATYPE(NodeId) // ок!
*****************************************************************************/

#pragma once
#include <QUuid>

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
    QUuidT(const char* uuidStr) noexcept : QUuid(QByteArray(uuidStr))
    {}
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
#ifdef __x86_64__
        COMPARE(((quint64 *) &u1)[0], ((quint64 *) &u2)[0])
        COMPARE(((quint64 *) &u1)[1], ((quint64 *) &u2)[1])
#else
        COMPARE(((quint32 *) &u1)[0], ((quint32 *) &u2)[0])
        COMPARE(((quint32 *) &u1)[1], ((quint32 *) &u2)[1])
        COMPARE(((quint32 *) &u1)[2], ((quint32 *) &u2)[2])
        COMPARE(((quint32 *) &u1)[3], ((quint32 *) &u2)[3])
#endif
        return 0;
    }

#undef COMPARE

    // equal
    template<typename UuidT1, typename UuidT2>
    static inline bool equal(const UuidT1& u1, const UuidT2& u2) noexcept
    {
        return (compare(u1, u2) == 0);
    }

    template<int NN>
    bool operator== (const QUuidT<NN>& u) noexcept {return equal(*this, u);}
    template<int NN>
    bool operator!= (const QUuidT<NN>& u) noexcept {return !equal(*this, u);}

    bool operator== (const QUuid& u) noexcept {return  equal(*this, u);}
    bool operator!= (const QUuid& u) noexcept {return !equal(*this, u);}

    template<int NN>
    bool operator< (const QUuidT<NN>& u) const noexcept {return (compare(*this, u) < 0);}
    template<int NN>
    bool operator> (const QUuidT<NN>& u) const noexcept {return (compare(*this, u) > 0);}

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
inline QDataStream& operator<< (QDataStream &s, const QUuidT<N> &u)
{
    return operator<< (s, static_cast<const QUuid&>(u));
}

template<int N>
inline QDataStream& operator>> (QDataStream &s, QUuidT<N> &u)
{
    return operator>> (s, static_cast<QUuid&>(u));
}

typedef QUuidT<0> QUuidEx;
