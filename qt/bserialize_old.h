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

  В модуле представлены функции и макросы механизма бинарной сериализации дан-
  ных. Данный механизм имеет поддержку версионности, что позволяет структурам
  имеющим различные версии организации данных выполнять корректную десериализа-
  цию данных друг друга.
  Текущая реализация выполнена с использование потоковых операторов Qt.
*****************************************************************************/

#pragma once
#include <QByteArray>
#include <QDataStream>
#include <QVector>


namespace bserial /*binary serialization*/ {

typedef QVector<QByteArray> RawVector;

/**
    Вспомогательные функции для обычных потоковых операторов
*/
template<typename T>
QDataStream& getFromStream(QDataStream& s, T& t)
{
    if (s.atEnd()) return s;
    RawVector rv;
    rv.reserve(2);
    s >> rv;
    t.fromRaw(rv);
    return s;
}

template<typename T>
QDataStream& putToStream(QDataStream& s, const T& t)
{
    s << t.toRaw();
    return s;
}

///**
//  Вспомогательные функции для потоковых операторов, используются
//  для записи/чтения структур container_ptr<> и clife_ptr<> из потока данных.
//*/
//template<typename PtrType>
//QDataStream& getPtrFromStream(QDataStream& s, PtrType& ptr)
//{
//    typedef typename PtrType::element_t element_t;

//    if (s.atEnd()) return s;
//    quint8 is_empty;
//    s >> is_empty;
//    if (is_empty) {
//        ptr = PtrType();
//        return s;
//    }
//    if (ptr.is_empty()) {
//        //ptr = PtrType(new element_t());
//        if (PtrType::IsContainer) {
//            // Для container_ptr<> дает более эффективное использование памяти.
//            ptr = PtrType::create_join_ptr();
//            new (ptr.get()) element_t();
//        }
//        else
//            ptr = PtrType(new element_t());
//    }

//    getFromStream(s, (*ptr));
//    return s;
//}

//template<typename PtrType>
//QDataStream& putPtrToStream(QDataStream& s, const PtrType& ptr)
//{
//    s << quint8(ptr.is_empty()); // is_empty
//    if (ptr.is_empty())
//        return s;

//    putToStream(s, (*ptr));
//    return s;
//}


/////**
////  Вспомогательная структура для функции getListFromStream()
////*/
////template<typename T> struct AddToList
////{
////    static inline void add(QList<T>& l, const T* p) {
////        l << *p;
////        delete p;
////    }
////    template<typename ListT>
////    static inline void add(ListT& l, const T* p) {
////        l.add(p);
////    }
////};

///**
//  Вспомогательные функции для потоковых операторов, используются
//  для записи/чтения структур lst::List<T> из потока данных.
//*/
//template<typename ListT>
//QDataStream& getListFromStream(QDataStream& s, ListT& list)
//{
//    //typedef typename ListCPtrT::element_t element_t;
//    typedef typename ListT::value_type value_type;

//    if (s.atEnd()) return s;
//    quint32 list_count; s >> list_count;
//    for (quint32 i = 0; i < list_count; ++i)
//    {
//        value_type* value = new value_type();
//        getFromStream(s, *value);
//        //AddToList<value_type>::add(list, value);
//        list.add(value);
//    }
//    return s;
//}

//template<typename ListT>
//QDataStream& putListToStream(QDataStream& s, const ListT& list)
//{
//    s << quint32(list.count());
//    for (int i = 0; i < list.count(); ++i)
//        putToStream(s, list.at(i));
//    return s;
//}

///**
//  Вспомогательные функции для потоковых операторов, используются
//  для записи/чтения структур lst::List<T> из потока данных,
//  где T унаследовано от counter_life<>.
//*/
//template<typename ListT>
//QDataStream& getLPtrListFromStream(QDataStream& s, ListT& list)
//{
//    //typedef typename ListCPtrT::element_t element_t;
//    typedef typename ListT::value_type value_type;

//    if (s.atEnd()) return s;
//    quint32 list_count; s >> list_count;
//    for (quint32 i = 0; i < list_count; ++i)
//    {
//        value_type* value = new value_type();
//        getFromStream(s, *value);
//        value->add_ref();
//        list.add(value);
//    }
//    return s;
//}


///**
//  Вспомогательные функции для потоковых операторов, используются
//  для записи/чтения структур container_ptr<lst::List<T>> из потока данных.
//*/
//template<typename ListCPtrT>
//QDataStream& getListCPtrFromStream(QDataStream& s, ListCPtrT& listCPtr)
//{
//    typedef typename ListCPtrT::element_t element_t;
//    typedef typename element_t::value_type value_type;

//    if (s.atEnd()) return s;
//    quint8 is_empty;
//    s >> is_empty;
//    if (is_empty) {
//        listCPtr = ListCPtrT();
//        return s;
//    }
//    if (listCPtr.is_empty())
//        listCPtr = ListCPtrT(new element_t());

//    quint32 list_count; s >> list_count;
//    for (quint32 i = 0; i < list_count; ++i)
//    {
//        value_type* value = new value_type();
//        getFromStream(s, *value);
//        listCPtr->add(value);
//        //AddToCPtrList<CPtrType>::add(cptrList, cptr);
//    }
//    return s;
//}

//template<typename ListCPtrT>
//QDataStream& putListCPtrToStream(QDataStream& s, const ListCPtrT& listCPtr)
//{
//    s << quint8(listCPtr.is_empty()); // is_empty
//    if (listCPtr.is_empty())
//        return s;

//    s << quint32(listCPtr->count());
//    for (int i = 0; i < listCPtr->count(); ++i)
//    {
//        putToStream(s, listCPtr->at(i));
//    }
//    return s;
//}

///**
//  Вспомогательная структура для функции getCPtrListFromStream()
//*/
//template<typename CPtrType> struct AddToCPtrList
//{
//    static inline void add(QList<CPtrType>& l, const CPtrType& p) {
//        l << p;
//    }
//    template<typename ListT>
//    static inline void add(ListT& l, const CPtrType& p) {
//        l.add(new CPtrType(p));
//    }
//};

///**
//  Вспомогательные функции для потоковых операторов, используются
//  для записи/чтения структур lst::List<container_ptr<T>> из потока данных.
//*/
//template<typename CPtrListT>
//QDataStream& getCPtrListFromStream(QDataStream& s, CPtrListT& cptrList)
//{
//    typedef typename CPtrListT::value_type CPtrType;
//
//    if (s.atEnd()) return s;
//    quint32 list_count; s >> list_count;
//    for (quint32 i = 0; i < list_count; ++i) {
//        CPtrType cptr;
//        getCPtrFromStream(s, cptr);
//        AddToCPtrList<CPtrType>::add(cptrList, cptr);
//    }
//    // Не сортируем список. Решение о сортировке должно приниматься
//    // в конкретной реализации, а не в обобщенной.
//    //cptr_list.sort();
//    return s;
//}
//
//template<typename CPtrListT>
//QDataStream& putCPtrListToStream(QDataStream& s, const CPtrListT& cptrList)
//{
//    s << quint32(cptrList.count());
//    for (int i = 0; i < cptrList.count(); ++i) {
//        putCPtrToStream(s, cptrList.at(i));
//    }
//    return s;
//}


#define DECLARE_B_SERIALIZE_FUNC \
    template<typename T> friend QDataStream& bserial::getFromStream(QDataStream&, T&); \
    template<typename T> friend QDataStream& bserial::putToStream(QDataStream&, const T&);
/*
    template<typename CPtrType> friend QDataStream& bserial::getPtrFromStream(QDataStream&, CPtrType&); \
    template<typename ListT> friend QDataStream& bserial::getListFromStream(QDataStream&, ListT&); \
    template<typename ListT> friend QDataStream& bserial::getLPtrListFromStream(QDataStream&, ListT&); \
    template<typename ListCPtrT> friend QDataStream& bserial::getListCPtrFromStream(QDataStream&, ListCPtrT&);
    //template<typename CPtrListT> friend QDataStream& bserial::getCPtrListFromStream(QDataStream&, CPtrListT&);
*/

/**
  Определение потоковых операторов учитывающих механизм совместимости по версиям.
*/
#define DEFINE_STREAM_OPERATORS(TYPE_NAME) \
    inline QDataStream& operator>> (QDataStream& s, TYPE_NAME& p) \
        {return bserial::getFromStream(s, p);} \
    inline QDataStream& operator<< (QDataStream& s, const TYPE_NAME& p) \
        {return bserial::putToStream(s, p);}


/**
  Определение потоковых операторов для контейнеров container_ptr<T> и clife_ptr<T>
*/
/*
#define DEFINE_PTR_STREAM_OPERATORS(PTR_TYPE_NAME) \
    inline QDataStream& operator>> (QDataStream& s, PTR_TYPE_NAME& p) \
        {return bserial::getPtrFromStream(s, p);} \
    inline QDataStream& operator<< (QDataStream& s, const PTR_TYPE_NAME& p) \
        {return bserial::putPtrToStream(s, p);}
*/

/**
  Определение потоковых операторов для конструкций вида: lst::List<T>
*/
/*
#define DEFINE_LIST_STREAM_OPERATORS(LIST_TYPE_NAME) \
    inline QDataStream& operator>> (QDataStream& s, LIST_TYPE_NAME& l) \
        {return bserial::getListFromStream(s, l);} \
    inline QDataStream& operator<< (QDataStream& s, const LIST_TYPE_NAME& l) \
        {return bserial::putListToStream(s, l);}
*/

///**
//  Определение потоковых операторов для конструкций вида: lst::List<container_ptr<T>>
//*/
/*
#define DEFINE_CPRTLIST_STREAM_OPERATORS(CPTRLIST_TYPE_NAME) \
    inline QDataStream& operator >> (QDataStream& s, CPTRLIST_TYPE_NAME& l) \
        {return bserial::getCPtrListFromStream(s, l);} \
    inline QDataStream& operator << (QDataStream& s, const CPTRLIST_TYPE_NAME& l) \
        {return bserial::putCPtrListToStream(s, l);}
*/

/**
  Определение потоковых операторов для конструкций вида: lst::List<T>,
  где T унаследовано от counter_life<>.
*/
/*
#define DEFINE_LPRTLIST_STREAM_OPERATORS(LPTRLIST_TYPE_NAME) \
    inline QDataStream& operator>> (QDataStream& s, LPTRLIST_TYPE_NAME& l) \
        {return bserial::getLPtrListFromStream(s, l);} \
    inline QDataStream& operator<< (QDataStream& s, const LPTRLIST_TYPE_NAME& l) \
        {return bserial::putListToStream(s, l);}
*/

/**
  Определение потоковых операторов для конструкций вида: container_ptr<lst::List<T>>
*/
/*
#define DEFINE_LISTCPTR_STREAM_OPERATORS(LISTCPTR_TYPE_NAME) \
    inline QDataStream& operator>> (QDataStream& s, LISTCPTR_TYPE_NAME& l) \
        {return bserial::getListCPtrFromStream(s, l);} \
    inline QDataStream& operator<< (QDataStream& s, const LISTCPTR_TYPE_NAME& l) \
        {return bserial::putListCPtrToStream(s, l);}
*/


} // namespace bserial


#ifndef Q_DATA_STREAM_VERSION
#define Q_DATA_STREAM_VERSION QDataStream::Qt_5_5
#endif

/**
  Макросы для работы с функциями сериализации toRaw(), fromRaw()

  Примеры заполнения тел функций:
  bserial::RawVector Class::toRaw()
  {
    //--- Version 1 ---
    B_SERIALIZE_V1(stream)
    stream << Class::field1;
    stream << Class::field2;
    stream << Class::field3;
    //--- Version 2 ---
    B_SERIALIZE_V2(stream) // Version 1
    stream << Class::newField4;
    stream << Class::newField5;
    //--- Version 3 ---
    B_SERIALIZE_V3(stream)
    ...
    B_SERIALIZE_RETURN
  }

  void Class::fromRaw(const bserial::RawVector& rawVector)
  {
    //--- Version 1 ---
    B_DESERIALIZE_V1(rawVector, stream)
    stream >> Class::field1;
    stream >> Class::field2;
    stream >> Class::field3;
    //--- Version 2 ---
    B_DESERIALIZE_V2(rawVector, stream)
    stream >> Class::newField4;
    stream >> Class::newField5;
    //--- Version 3 ---
    B_DESERIALIZE_V3(rawVector, stream)
    ...
    B_DESERIALIZE_END
  }
*/
#define B_SERIALIZE_V1(STREAM) \
    bserial::RawVector to__raw__vect__; \
    to__raw__vect__.reserve(2); \
    { QByteArray to__raw__ba__; \
      { QDataStream STREAM(&to__raw__ba__, QIODevice::WriteOnly);  \
        STREAM.setVersion(Q_DATA_STREAM_VERSION);

#define B_SERIALIZE_N(STREAM) \
      } \
      to__raw__vect__.append(to__raw__ba__); \
    } \
    { QByteArray to__raw__ba__; \
      { QDataStream STREAM(&to__raw__ba__, QIODevice::WriteOnly); \
        STREAM.setVersion(Q_DATA_STREAM_VERSION);

#define B_SERIALIZE_V2(STREAM) SERIALIZE_N(STREAM)
#define B_SERIALIZE_V3(STREAM) SERIALIZE_N(STREAM)
#define B_SERIALIZE_V4(STREAM) SERIALIZE_N(STREAM)
#define B_SERIALIZE_V5(STREAM) SERIALIZE_N(STREAM)

#define B_SERIALIZE_RETURN \
      } \
      to__raw__vect__.append(to__raw__ba__); \
    } \
    return std::move(to__raw__vect__);

#define B_DESERIALIZE_V1(VECT, STREAM) \
    if (VECT.count() >= 1) { \
        const QByteArray& ba__from_raw__ = VECT.at(0); \
        QDataStream STREAM(ba__from__raw__); \
        STREAM.setVersion(Q_DATA_STREAM_VERSION);

#define B_DESERIALIZE_N(N, VECT, STREAM) \
    } if (VECT.count() >= N) { \
        const QByteArray& ba__from__raw__ = VECT.at(N - 1); \
        QDataStream STREAM(ba__from__raw__); \
        STREAM.setVersion(Q_DATA_STREAM_VERSION);

#define B_DESERIALIZE_V2(VECT, STREAM) DESERIALIZE_N(2, VECT, STREAM)
#define B_DESERIALIZE_V3(VECT, STREAM) DESERIALIZE_N(3, VECT, STREAM)
#define B_DESERIALIZE_V4(VECT, STREAM) DESERIALIZE_N(4, VECT, STREAM)
#define B_DESERIALIZE_V5(VECT, STREAM) DESERIALIZE_N(5, VECT, STREAM)

#define B_DESERIALIZE_END }
