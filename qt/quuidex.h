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

#include "break_point.h"
#include <QUuid>


///**
//  Агалог структуры win GUID
//*/
//struct QGUID
//{
//    ulong  Data1;
//    ushort Data2;
//    ushort Data3;
//    uchar  Data4[8];
////     QGUID(ulong  data1, ushort data2, ushort data3, uchar data4[8])
////         : data1(data1), Data2(data2), Data3(data3), Data4(data4)
////     {}
//};



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

#ifdef __x86_64__
    // equal
    template<typename UuidT1, typename UuidT2>
    static inline bool equal(const UuidT1& u1, const UuidT2& u2) noexcept
    {
        static_assert(sizeof(u1) == 16, "Parameter u1 should be size 16 byte");
        static_assert(sizeof(u2) == 16, "Parameter u2 should be size 16 byte");
        return ( ((quint64 *) &u1)[0] == ((quint64 *) &u2)[0]
              && ((quint64 *) &u1)[1] == ((quint64 *) &u2)[1] );
    }

    // compare
    template<typename UuidT1, typename UuidT2>
    static inline int compare(const UuidT1& u1, const UuidT2& u2) noexcept
    {
        static_assert(sizeof(u1) == 16, "Parameter u1 should be size 16 byte");
        static_assert(sizeof(u2) == 16, "Parameter u2 should be size 16 byte");
        COMPARE(((quint64 *) &u1)[0], ((quint64 *) &u2)[0])
        COMPARE(((quint64 *) &u1)[1], ((quint64 *) &u2)[1])
        return 0;
    }
#else
    // equal
    template<typename UuidT1, typename UuidT2>
    static inline bool equal(const UuidT1& u1, const UuidT2& u2) Q_DECL_NOTHROW
    {
        static_assert(sizeof(u1) == 16, "Parameter u1 should be size 16 byte");
        static_assert(sizeof(u2) == 16, "Parameter u2 should be size 16 byte");
        return ( ((quint32 *) &u1)[0] == ((quint32 *) &u2)[0]
              && ((quint32 *) &u1)[1] == ((quint32 *) &u2)[1]
              && ((quint32 *) &u1)[2] == ((quint32 *) &u2)[2]
              && ((quint32 *) &u1)[3] == ((quint32 *) &u2)[3] );
    }

    // compare
    template<typename UuidT1, typename UuidT2>
    static inline int compare(const UuidT1& u1, const UuidT2& u2) Q_DECL_NOTHROW
    {
//        if (((quint32 *) &u1)[0] == ((quint32 *) &u2)[0])
//        {
//            if (((quint32 *) &u1)[1] == ((quint32 *) &u2)[1])
//            {
//                if (((quint32 *) &u1)[2] == ((quint32 *) &u2)[2])
//                {
//                    if (((quint32 *) &u1)[3] == ((quint32 *) &u2)[3])
//                        return 0;
//                    else
//                        return (((quint32 *) &u1)[3] > ((quint32 *) &u2)[3]) ? 1 : -1;
//                }
//                return (((quint32 *) &u1)[2] > ((quint32 *) &u2)[2]) ? 1 : -1;
//            }
//            return (((quint32 *) &u1)[1] > ((quint32 *) &u2)[1]) ? 1 : -1;
//        }
//        return (((quint32 *) &u1)[0] > ((quint32 *) &u2)[0]) ? 1 : -1;

        static_assert(sizeof(u1) == 16, "Parameter u1 should be size 16 byte");
        static_assert(sizeof(u2) == 16, "Parameter u2 should be size 16 byte");
        COMPARE(((quint32 *) &u1)[0], ((quint32 *) &u2)[0])
        COMPARE(((quint32 *) &u1)[1], ((quint32 *) &u2)[1])
        COMPARE(((quint32 *) &u1)[2], ((quint32 *) &u2)[2])
        COMPARE(((quint32 *) &u1)[3], ((quint32 *) &u2)[3])
        return 0;
    }
#endif

#undef COMPARE

    template<int NN>
    bool operator== (const QUuidT<NN>& u) noexcept {return equal(*this, u);}
    template<int NN>
    bool operator!= (const QUuidT<NN>& u) noexcept {return !equal(*this, u);}

    bool operator== (const QUuid& u) noexcept {return  equal(*this, u);}
    bool operator!= (const QUuid& u) noexcept {return !equal(*this, u);}
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



/*
#define DEF_IDENTIFIER(TYPE, NAME, L, W1, W2, B1, B2, B3, B4, B5, B6, B7, B8) \
    extern "C" const __declspec(selectany) TYPE NAME(L, W1, W2, B1, B2, B3, B4, B5, B6, B7, B8);
*/


