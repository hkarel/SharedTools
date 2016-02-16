
#pragma once

#include <QUuid>
//#include <QList>
//#include "regmtypes.h"


/**
  Агалог структуры win GUID
*/
struct QGUID
{
    ulong  Data1;
    ushort Data2;
    ushort Data3;
    uchar  Data4[8];
//     QGUID(ulong  data1, ushort data2, ushort data3, uchar data4[8])
//         : data1(data1), Data2(data2), Data3(data3), Data4(data4)
//     {}
};



/**
  Класс QUuidT полностью повторяет функционал QUuid, за исключением
  переопределенных операторов равенства и неравенства, и добавленного
  оператора сравнения.
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
*/
template<int N> struct QUuidT : public QUuid
{
    QUuidT() : QUuid() {init_debug_guid();}
    QUuidT(const QUuid& u) : QUuid(u) {init_debug_guid();}
    QUuidT(const QUuidT& u) : QUuid(u) {init_debug_guid();}
    QUuidT(const QGUID& guid) {
        data1 = guid.Data1;
        data2 = guid.Data2;
        data3 = guid.Data3;
        for(int i = 0; i < 8; ++i)
            data4[i] = guid.Data4[i];
        init_debug_guid();
    }
    QUuidT(uint l, ushort w1, ushort w2, uchar b1, uchar b2, uchar b3, uchar b4, uchar b5, uchar b6, uchar b7, uchar b8)
        : QUuid(l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
    {init_debug_guid();}
    QUuidT& operator= (const QUuid &u) {
        QUuid::operator= (u);
        init_debug_guid();
        return *this;
    }

    static inline bool equalL(const void* uuid1, const void* uuid2) {
        // Не протестировано
        _CrtDbgBreak();

        const ulong* uuid1_ = static_cast<const ulong*>(uuid1);
        const ulong* uuid2_ = static_cast<const ulong*>(uuid2);
        return (uuid1_[0] == uuid2_[0] && uuid1_[1] == uuid2_[1] && uuid1_[2] == uuid2_[2] && uuid1_[3] == uuid2_[3]);
    }

    template<typename UuidT1, typename UuidT2>
    static inline bool equal(const UuidT1& uuid1, const UuidT2& uuid2) {
        return equalL(&uuid1, &uuid2);
//         return (
//             ((ulong *) &guid1)[0] == ((ulong *) &guid2)[0] &&
//             ((ulong *) &guid1)[1] == ((ulong *) &guid2)[1] &&
//             ((ulong *) &guid1)[2] == ((ulong *) &guid2)[2] &&
//             ((ulong *) &guid1)[3] == ((ulong *) &guid2)[3]);
    }

    static inline int compareL(const void* uuid1, const void* uuid2)
    {
        const ulong* uuid1_ = static_cast<const ulong*>(uuid1);
        const ulong* uuid2_ = static_cast<const ulong*>(uuid2);
        if (uuid1_[0] == uuid2_[0]) {
            if (uuid1_[1] == uuid2_[1]) {
                if (uuid1_[2] == uuid2_[2]) {
                    if (uuid1_[3] == uuid2_[3])
                        return 0;
                    else
                        return (uuid1_[3] > uuid2_[3]) ? 1 : -1;
                }
                return (uuid1_[2] > uuid2_[2]) ? 1 : -1;
            }
            return (uuid1_[1] > uuid2_[1]) ? 1 : -1;
        }
        return (uuid1_[0] > uuid2_[0]) ? 1 : -1;
    }

    template<typename UuidT1, typename UuidT2>
    static inline int compare(const UuidT1& uuid1, const UuidT2& uuid2)
    {
        // Не протестировано
        _CrtDbgBreak();

        return compareL(&uuid1, &uuid2);

//         if (((ulong *) &guid1)[0] == ((ulong *) &guid2)[0]) {
//             if (((ulong *) &guid1)[1] == ((ulong *) &guid2)[1]) {
//                 if (((ulong *) &guid1)[2] == ((ulong *) &guid2)[2]) {
//                     if (((ulong *) &guid1)[3] == ((ulong *) &guid2)[3])
//                         return 0;
//                     else
//                         return (((ulong *) &guid1)[3] > ((ulong *) &guid2)[3]) ? 1 : -1;
//                 }
//                 return (((ulong *) &guid1)[2] > ((ulong *) &guid2)[2]) ? 1 : -1;
//             }
//             return (((ulong *) &guid1)[1] > ((ulong *) &guid2)[1]) ? 1 : -1;
//         }
//         return (((ulong *) &guid1)[0] > ((ulong *) &guid2)[0]) ? 1 : -1;
    }


    //friend bool operator == (const QUuidT<N> &u1, const QUuidT<N> &u2) {
    //    return QUuidT::equal(u1, u2);
    //}
    //template<typename UuidType1, typename UuidType2>
    //friend inline bool operator == (const UuidType1 &u1, const UuidType2 &u2) {
    //    return QUuidT::equal(u1, u2);
    //}
    template<typename UuidT>
    friend inline bool operator == (const UuidT &u1, const QUuidT<N> &u2) {
        return QUuidT::equal(u1, u2);
    }

    //friend bool operator != (const QUuidT<N> &u1, const QUuidT<N> &u2) {
    //    return !QUuidT::equal(u1, u2);
    //}
    //template<typename UuidType1, typename UuidType2>
    //friend inline bool operator != (const UuidType1 &u1, const UuidType2 &u2) {
    //    return !QUuidT::equal(u1, u2);
    //}
    template<typename UuidT>
    friend inline bool operator != (const UuidT &u1, const QUuidT<N> &u2) {
        return !QUuidT::equal(u1, u2);
    }

    friend inline QDataStream& operator << (QDataStream &s, const QUuidT<N> &u) {
        return operator << (s, static_cast<const QUuid&>(u));
    }
    friend inline QDataStream& operator >> (QDataStream &s, QUuidT<N> &u) {
#ifndef NDEBUG
        operator >> (s, static_cast<QUuid&>(u));
        u.init_debug_guid();
        return s;
#else
        return operator >> (s, static_cast<QUuid&>(u));
#endif
    }

#ifdef Q_OS_WIN
    // Отладочный параметр, используется для удобства работы в отладчике MS VisualStudio.
    GUID guid_;
#endif

    inline void init_debug_guid() {
#ifdef Q_OS_WIN
        guid_ = (GUID) *this;
#endif
    }
};

typedef QUuidT<0> QUuidEx;

#define DEF_IDENTIFIER(TYPE, NAME, L, W1, W2, B1, B2, B3, B4, B5, B6, B7, B8) \
    extern "C" const __declspec(selectany) TYPE NAME(L, W1, W2, B1, B2, B3, B4, B5, B6, B7, B8);
