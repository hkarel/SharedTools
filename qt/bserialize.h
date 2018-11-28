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

  В модуле представлены функции и макросы механизма бинарной сериализации
  данных. Данный механизм имеет поддержку версионности, что позволяет
  структурам имеющим различные версии организации данных выполнять корректную
  десериализацию данных друг друга.
  Текущая реализация выполнена с использование потоковых операторов Qt.
*****************************************************************************/

#pragma once

#include "list.h"
#include "clife_base.h"
#include "clife_ptr.h"
#include "break_point.h"

#include <QByteArray>
#include <QDataStream>
#include <QVector>
#include <stdexcept>
#include <type_traits>

namespace bserial /*binary serialization*/ {

/**
  Структура ByteArray нужна для переопределения потокового оператора '>>'
  для QByteArray. Переопределенный оператор '>>' менее универсальный, но
  дает преимущество по производительности.
*/
struct ByteArray : QByteArray
{
    ByteArray() noexcept : QByteArray() {}
    ByteArray(const QByteArray& ba) noexcept : QByteArray(ba) {}
    ByteArray(const ByteArray&  ba) noexcept : QByteArray(ba) {}

    ByteArray& operator= (const ByteArray& ba) noexcept
        {return (ByteArray&)QByteArray::operator= (ba);}

    ByteArray& operator= (const QByteArray& ba) noexcept
        {return (ByteArray&)QByteArray::operator= (ba);}

    ByteArray& operator= (const char* str) noexcept
        {return (ByteArray&)QByteArray::operator= (str);}

    // Функции используются в макросах B_SERIALIZE_Vx
    void reserve() {}
    void reserve(size_t size) {QByteArray::reserve(int(size));}
};
QDataStream& operator>> (QDataStream&, ByteArray&);
inline QDataStream& operator<< (QDataStream& s, const ByteArray& ba)
    {return operator<< (s, static_cast<const QByteArray&>(ba));}

typedef QVector<ByteArray> RawVector;

template<typename T>
struct not_enum_type : std::enable_if<!std::is_enum<T>::value, int> {};
template<typename T>
struct is_enum_type : std::enable_if<std::is_enum<T>::value, int> {};

/**
  Вспомогательные функции для обычных потоковых операторов
*/
template<typename T>
QDataStream& getFromStream(QDataStream& s, T& t,
                           typename not_enum_type<T>::type = 0)
{
    if (s.atEnd())
        return s;

    quint8 size;
    s >> size;
    RawVector rv {int(size)};
    for(quint8 i = 0; i < size; ++i)
    {
        ByteArray ba;
        s >> ba;
        rv[i] = ba;
    }
    t.fromRaw(rv);
    return s;
}
template<typename T>
QDataStream& putToStream(QDataStream& s, const T& t,
                         typename not_enum_type<T>::type = 0)
{
    const RawVector rv = t.toRaw();
    if (rv.size() > 255)
        throw std::logic_error(
            "Limit exceeded on number of versions for b-serialization (255)");
    s << quint8(rv.size());
    for (const ByteArray& ba : rv)
        s << ba;
    return s;
}

/**
  Вспомогательные функции для enum-типов
*/
template<typename T>
QDataStream& getFromStream(QDataStream& s, T& t,
                           typename is_enum_type<T>::type = 0)
{
    static_assert(std::is_same<typename std::underlying_type<T>::type, quint32>::value,
                  "Base type of enum must be 'unsigned int'");
    if (s.atEnd())
        return s;

    quint32 val;
    s >> val;
    t = static_cast<T>(val);
    return s;
}

template<typename T>
QDataStream& putToStream(QDataStream& s, const T t,
                         typename is_enum_type<T>::type = 0)
{
    static_assert(std::is_same<typename std::underlying_type<T>::type, quint32>::value,
                  "Base type of enum must be 'unsigned int'");

    s << static_cast<quint32>(t);
    return s;
}

/**
  Вспомогательные функции для потоковых операторов, используются
  для записи/чтения структур clife_ptr<T> из потока данных.
*/
template<typename T>
QDataStream& getFromStream(QDataStream& s, clife_ptr<T>& ptr)
{
    static_assert(std::is_base_of<clife_base, T>::value,
                  "Class T must be derived from clife_base");
    if (s.atEnd())
        return s;

    bool empty;
    s >> empty;
    if (empty)
    {
        ptr = clife_ptr<T>();
        return s;
    }
    if (ptr.empty())
        ptr = clife_ptr<T>(new T());

    getFromStream(s, (*ptr));
    return s;
}

template<typename T>
QDataStream& putToStream(QDataStream& s, const clife_ptr<T>& ptr)
{
    s << ptr.empty();
    if (ptr.empty())
        return s;

    putToStream(s, (*ptr));
    return s;
}

template<typename T>
struct derived_from_clife_base : std::enable_if<std::is_base_of<clife_base, T>::value, int> {};
template<typename T>
struct not_derived_from_clife_base : std::enable_if<!std::is_base_of<clife_base, T>::value, int> {};

/**
  Вспомогательные функции для потоковых операторов, используются
  для записи/чтения структур lst::List<T> из потока данных,
  где T унаследовано от clife_base.
*/
template<
    typename T,
    typename Compare,
    typename Allocator
>
QDataStream& getFromStream(QDataStream& s, lst::List<T, Compare, Allocator>& list,
                           typename derived_from_clife_base<T>::type = 0)
{
    /* Эта функция используется когда T унаследовано от clife_base */
    if (s.atEnd())
        return s;

    quint32 listCount; s >> listCount;
    for (quint32 i = 0; i < listCount; ++i)
    {
        // Отладить
        break_point

        typedef lst::List<T, Compare, Allocator> ListType;
        typename ListType::ValueType* value = list.allocator().create();
        if (value->clife_count == 0)
            value->add_ref();
        getFromStream(s, *value);
        list.add(value);
    }
    return s;
}

template<
    typename T,
    typename Compare,
    typename Allocator
>
QDataStream& getFromStream(QDataStream& s, lst::List<T, Compare, Allocator>& list,
                           typename not_derived_from_clife_base<T>::type = 0)

{
    /* Эта функция используется когда T НЕ унаследовано от clife_base */
    if (s.atEnd())
        return s;

    quint32 listCount; s >> listCount;
    for (quint32 i = 0; i < listCount; ++i)
    {
        typedef lst::List<T, Compare, Allocator> ListType;
        typename ListType::ValueType* value = list.allocator().create();
        getFromStream(s, *value);
        list.add(value);
    }
    return s;
}

template<
    typename T,
    typename Compare,
    typename Allocator
>
QDataStream& putToStream(QDataStream& s, const lst::List<T, Compare, Allocator>& list)
{
    s << quint32(list.count());
    for (int i = 0; i < list.count(); ++i)
        putToStream(s, list.at(i));
    return s;
}

} // namespace bserial

#define DECLARE_B_SERIALIZE_FRIENDS \
    template<typename T> \
    friend QDataStream& bserial::getFromStream(QDataStream&, T&, \
                                               typename not_enum_type<T>::type); \
    template<typename T> \
    friend QDataStream& bserial::getFromStream(QDataStream&, T&, \
                                               typename is_enum_type<T>::type); \
    template<typename T> \
    friend QDataStream& bserial::getFromStream(QDataStream& s, clife_ptr<T>&); \
    \
    template<typename T, typename Compare, typename Allocator> \
    friend QDataStream& bserial::getFromStream(QDataStream& s, lst::List<T, Compare, Allocator>&, \
                                               typename derived_from_clife_base<T>::type); \
    \
    template<typename T, typename Compare, typename Allocator> \
    friend QDataStream& bserial::getFromStream(QDataStream& s, lst::List<T, Compare, Allocator>&, \
                                               typename not_derived_from_clife_base<T>::type); \
    template<typename T> \
    friend QDataStream& bserial::putToStream(QDataStream&, const T&, \
                                             typename not_enum_type<T>::type); \
    template<typename T> \
    friend QDataStream& bserial::putToStream(QDataStream&, const T&, \
                                             typename is_enum_type<T>::type); \
    template<typename T> \
    friend QDataStream& bserial::putToStream(QDataStream& s, const clife_ptr<T>&); \
    \
    template<typename T, typename Compare, typename Allocator> \
    friend QDataStream& bserial::putToStream(QDataStream& s, const lst::List<T, Compare, Allocator>&);

#define DECLARE_B_SERIALIZE_FUNC \
    bserial::RawVector toRaw() const; \
    void fromRaw(const bserial::RawVector&); \
    DECLARE_B_SERIALIZE_FRIENDS

/**
  --- Устаревшая реализация ---
  Определение потоковых операторов учитывающих механизм совместимости по версиям.

#define DEFINE_STREAM_OPERATORS(TYPE_NAME) \
    inline QDataStream& operator>> (QDataStream& s, TYPE_NAME& p) \
        {return bserial::getFromStream(s, p);} \
    inline QDataStream& operator<< (QDataStream& s, const TYPE_NAME& p) \
        {return bserial::putToStream(s, p);}
*/

/**
  Определение обобщенных потоковых операторов учитывающих механизм совместимости
  по версиям.
  Примечание: для того чтобы компилятор мог корректно выполнить инстанциирование
  шаблонных параметров требуется соблюдение правил ADL-поиска (или поиска Кёнига),
  поэтому макрос DEFINE_B_SERIALIZE_STREAM_OPERATORS обязательно должен нахо-
  диться внутри пространства имен структур для которых выполняется сериализация.
*/
#define DEFINE_B_SERIALIZE_STREAM_OPERATORS \
    template<typename T> \
    inline QDataStream& operator>> (QDataStream& s, T& p) \
        {return bserial::getFromStream<T>(s, p);} \
    template<typename T> \
    inline QDataStream& operator<< (QDataStream& s, const T& p) \
        {return bserial::putToStream<T>(s, p);} \
    template<typename T> \
    inline QDataStream& operator>> (QDataStream& s, clife_ptr<T>& p) \
        {return bserial::getFromStream<T>(s, p);} \
    template<typename T> \
    inline QDataStream& operator<< (QDataStream& s, const clife_ptr<T>& p) \
        {return bserial::putToStream<T>(s, p);} \
    template<typename T, typename Compare, typename Allocator> \
    inline QDataStream& operator>> (QDataStream& s, lst::List<T, Compare, Allocator>& p) \
        {return bserial::getFromStream<T, Compare, Allocator>(s, p);} \
    template<typename T, typename Compare, typename Allocator> \
    inline QDataStream& operator<< (QDataStream& s, const lst::List<T, Compare, Allocator>& p) \
        {return bserial::putToStream<T, Compare, Allocator>(s, p);}


typedef bserial::ByteArray BByteArray;

#ifndef QDATASTREAM_VERSION
#  if QT_VERSION >= 0x050000
#    define QDATASTREAM_VERSION QDataStream::Qt_5_5
#  else
#    define QDATASTREAM_VERSION QDataStream::Qt_4_8
#  endif
#endif


/**
  Макросы для работы с функциями сериализации toRaw(), fromRaw()

  Ниже приведены примеры реализации этих функций. Обратите внимание, что в мак-
  росах B_SERIALIZE_Vx используются два параметра, причем второй параметр опцио-
  нальный. Он определяет размер резервирования памяти для процесса сериализации.
  Если есть возможность получить размер сериализуемой структуры (хотя бы прибли-
  зительный размер), то настоятельно рекомендуется указывать его в качестве вто-
  рого параметра - это сократит накладные расходы на повторное выделения памяти.
  Максимальное количество версий синхронизации для структуры равно 255.

  bserial::RawVector Class::toRaw()
  {
    //--- Version 1 ---
    B_SERIALIZE_V1(stream, sizeof(Class))
    stream << Class::field1;
    stream << Class::field2;
    stream << Class::field3;
    //--- Version 2 ---
    B_SERIALIZE_V2(stream)
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
#define B_SERIALIZE_V1(STREAM, RESERVE...) \
    bserial::RawVector to__raw__vect__; \
    to__raw__vect__.reserve(2); \
    { bserial::ByteArray to__raw__ba__; \
      to__raw__ba__.reserve(RESERVE); \
      { QDataStream STREAM(&to__raw__ba__, QIODevice::WriteOnly);  \
        STREAM.setVersion(QDATASTREAM_VERSION);

#define B_SERIALIZE_N(STREAM, RESERVE...) \
      } \
      to__raw__vect__.append(to__raw__ba__); \
    } \
    { bserial::ByteArray to__raw__ba__; \
      to__raw__ba__.reserve(RESERVE); \
      { QDataStream STREAM(&to__raw__ba__, QIODevice::WriteOnly); \
        STREAM.setVersion(QDATASTREAM_VERSION);

#define B_SERIALIZE_V2(STREAM, RESERVE...) B_SERIALIZE_N(STREAM, RESERVE)
#define B_SERIALIZE_V3(STREAM, RESERVE...) B_SERIALIZE_N(STREAM, RESERVE)
#define B_SERIALIZE_V4(STREAM, RESERVE...) B_SERIALIZE_N(STREAM, RESERVE)
#define B_SERIALIZE_V5(STREAM, RESERVE...) B_SERIALIZE_N(STREAM, RESERVE)

#define B_SERIALIZE_RETURN \
      } \
      to__raw__vect__.append(to__raw__ba__); \
    } \
    return std::move(to__raw__vect__);

#define B_DESERIALIZE_V1(VECT, STREAM) \
    if (VECT.count() >= 1) { \
        const bserial::ByteArray& ba__from__raw__ = VECT.at(0); \
        QDataStream STREAM {(bserial::ByteArray*)&ba__from__raw__, \
                            QIODevice::ReadOnly | QIODevice::Unbuffered}; \
        STREAM.setVersion(QDATASTREAM_VERSION);

#define B_DESERIALIZE_N(N, VECT, STREAM) \
    } if (VECT.count() >= N) { \
        const bserial::ByteArray& ba__from__raw__ = VECT.at(N - 1); \
        QDataStream STREAM {(bserial::ByteArray*)&ba__from__raw__, \
                            QIODevice::ReadOnly | QIODevice::Unbuffered}; \
        STREAM.setVersion(QDATASTREAM_VERSION);

#define B_DESERIALIZE_V2(VECT, STREAM) B_DESERIALIZE_N(2, VECT, STREAM)
#define B_DESERIALIZE_V3(VECT, STREAM) B_DESERIALIZE_N(3, VECT, STREAM)
#define B_DESERIALIZE_V4(VECT, STREAM) B_DESERIALIZE_N(4, VECT, STREAM)
#define B_DESERIALIZE_V5(VECT, STREAM) B_DESERIALIZE_N(5, VECT, STREAM)

#define B_DESERIALIZE_END }
