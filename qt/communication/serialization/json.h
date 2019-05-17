/*****************************************************************************
  The MIT License

  Copyright © 2018 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  В модуле представлены функции и макросы механизма json сериализации данных

*****************************************************************************/

#pragma once

#include "defmac.h"
#include "list.h"
#include "clife_base.h"
#include "clife_ptr.h"
#include "break_point.h"
#include "logger/logger.h"
#include "qt/quuidex.h"
#include "qt/logger/logger_operators.h"
#include "qt/communication/serialization/sresult.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <QtGlobal>
#include <QDateTime>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QVector>
#include <QStack>
#include <type_traits>

namespace communication {
namespace serialization {
namespace json {

using namespace rapidjson;
template <typename T> Reader& readQArray(Reader&, T&);

class Reader
{
public:
    Reader();
    ~Reader();

    SResult result() const;

    // Parse json
    bool parse(const QByteArray& json);
    bool hasParseError() const {return _hasParseError;}

    Reader& member(const char* name, bool optional = false);
    quint64 jsonIndex() const {return _jsonIndex;}

    Reader& startObject();
    Reader& endObject();

    Reader& startArray(SizeType* size);
    Reader& endArray();

    Reader& setNull();

    Reader& operator& (bool&);
    Reader& operator& (qint8&);
    Reader& operator& (quint8&);
    Reader& operator& (qint16&);
    Reader& operator& (quint16&);
    Reader& operator& (qint32&);
    Reader& operator& (quint32&);
    Reader& operator& (qint64&);
    Reader& operator& (quint64&);
    Reader& operator& (double&);
    Reader& operator& (float&);
    Reader& operator& (QByteArray&);
    Reader& operator& (QString&);
    Reader& operator& (QUuid&);
    Reader& operator& (QDate&);
    Reader& operator& (QTime&);
    Reader& operator& (QDateTime&);

    template <typename T> Reader& operator& (T& t);
    template <typename T> Reader& operator& (QList<T>&);
    template <typename T> Reader& operator& (QVector<T>&);
    template <typename T> Reader& operator& (clife_ptr<T>&);
    template <int N>      Reader& operator& (QUuidT<N>&);

    template<typename T, typename Compare, typename Allocator>
    Reader& operator& (lst::List<T, Compare, Allocator>&);

    bool isReader() const {return true;}
    bool isWriter() const {return false;}

private:
    struct StackItem
    {
        enum State
        {
            BeforeStart, // An object/array is in the stack but it is not yet called
                         // by StartObject()/StartArray().
            Started,     // An object/array is called by StartObject()/StartArray().
            Closed       // An array is closed after read all element, but before EndArray().
        };

        StackItem() = default;
        StackItem(const Value* value, State state, const char* name = 0)
            : value(value), state(state), name(name)
        {}

        QByteArray name;
        const Value* value;
        State state;
        SizeType index = {0}; // For array iteration
    };
    typedef QStack<StackItem> Stack;

    int error() const {return _error;}
    void setError(int val, bool optional = false);

    QByteArray stackFieldName() const;

private:
    DISABLE_DEFAULT_COPY(Reader)
    void Next();

    Document _document;
    Stack _stack;

    int _error = {0};
    bool _hasParseError = {false};

    quint64 _jsonIndex = {0};
    QByteArray _jsonContent;

    template <typename T> friend Reader& readQArray(Reader&, T&);
};

class Writer
{
public:
    Writer();
    ~Writer();

    // Obtains the serialized JSON string.
    const char* getString() const;

    Writer& member(const char* name, bool /*optional*/ = false);

    Writer& startObject();
    Writer& endObject();

    Writer& startArray(SizeType* size = 0);
    Writer& endArray();

    Writer& setNull();

    Writer& operator& (const bool);
    Writer& operator& (const qint8);
    Writer& operator& (const quint8);
    Writer& operator& (const qint16);
    Writer& operator& (const quint16);
    Writer& operator& (const qint32);
    Writer& operator& (const quint32);
    Writer& operator& (const qint64);
    Writer& operator& (const quint64);
    Writer& operator& (const double);
    Writer& operator& (const float);
    Writer& operator& (const QByteArray&);
    Writer& operator& (const QString&);
    Writer& operator& (const QUuid&);
    Writer& operator& (const QDate&);
    Writer& operator& (const QTime&);
    Writer& operator& (const QDateTime&);

    template <typename T> Writer& operator& (const T& t);
    template <typename T> Writer& operator& (const QList<T>&);
    template <typename T> Writer& operator& (const QVector<T>&);
    template <typename T> Writer& operator& (const clife_ptr<T>&);
    template <int N>      Writer& operator& (const QUuidT<N>&);

    template<typename T, typename Compare, typename Allocator>
    Writer& operator& (const lst::List<T, Compare, Allocator>&);

    bool isReader() const {return false;}
    bool isWriter() const {return true;}

private:
    DISABLE_DEFAULT_COPY(Writer)

    StringBuffer _stream;
    rapidjson::Writer<StringBuffer> _writer;
};


//---------------------------- Reader, Writer --------------------------------

namespace {

template<typename T>
struct not_enum_type : std::enable_if<!std::is_enum<T>::value, int> {};
template<typename T>
struct is_enum_type : std::enable_if<std::is_enum<T>::value, int> {};

template<typename T>
struct derived_from_clife_base : std::enable_if<std::is_base_of<clife_base, T>::value, int> {};
template<typename T>
struct not_derived_from_clife_base : std::enable_if<!std::is_base_of<clife_base, T>::value, int> {};

template <typename Packer, typename T>
Packer& operatorAmp(Packer& p, T& t, typename not_enum_type<T>::type = 0)
{
    t.jserialize(p);
    return p;
}

template <typename T>
Reader& operatorAmp(Reader& r, T& t, typename is_enum_type<T>::type = 0)
{
    static_assert(std::is_same<typename std::underlying_type<T>::type, quint32>::value,
                  "Base type of enum must be 'unsigned int'");

    quint32 val;
    r & val;
    t = static_cast<T>(val);
    return r;
}

template <typename T>
Writer& operatorAmp(Writer& w, const T t, typename is_enum_type<T>::type = 0)
{
    static_assert(std::is_same<typename std::underlying_type<T>::type, quint32>::value,
                  "Base type of enum must be 'unsigned int'");

    quint32 val = static_cast<quint32>(t);
    w & val;
    return w;
}

template<typename T, typename Compare, typename Allocator>
Reader& operatorAmp(Reader& r, lst::List<T, Compare, Allocator>& list,
                    typename derived_from_clife_base<T>::type = 0)
{
    /* Эта функция используется когда T унаследовано от clife_base */
    list.clear();
    SizeType count;
    r.startArray(&count);
    for (SizeType i = 0; i < count; ++i)
    {
        typedef lst::List<T, Compare, Allocator> ListType;
        typename ListType::ValueType* value = list.allocator().create();
        if (value->clife_count() == 0)
            value->add_ref();
        r & (*value);
        list.add(value);
    }
    return r.endArray();
}

template<typename T, typename Compare, typename Allocator>
Reader& operatorAmp(Reader& r, lst::List<T, Compare, Allocator>& list,
                    typename not_derived_from_clife_base<T>::type = 0)
{
    /* Эта функция используется когда T НЕ унаследовано от clife_base */
    list.clear();
    SizeType count;
    r.startArray(&count);
    for (SizeType i = 0; i < count; ++i)
    {
        typedef lst::List<T, Compare, Allocator> ListType;
        typename ListType::ValueType* value = list.allocator().create();
        r & (*value);
        list.add(value);
    }
    return r.endArray();
}

} // namespace

template <typename T>
Reader& Reader::operator& (T& t)
{
    if (error())
        return *this;

    Reader& r = const_cast<Reader&>(*this);
    return operatorAmp(r, t);
}

template <typename T>
Writer& Writer::operator& (const T& t)
{
    Writer& w = const_cast<Writer&>(*this);
    T& t_ = const_cast<T&>(t);
    return operatorAmp(w, t_);
}

template <typename T>
Reader& readQArray(Reader& r, T& arr)
{
    if (r.error())
        return r;

    arr.clear();
    SizeType count;
    r.startArray(&count);
    for (SizeType i = 0; i < count; ++i)
    {
        typename T::value_type t;
        r & t;
        arr.append(t);
    }
    return r.endArray();
}

template <typename T>
Writer& writeQArray(Writer& w, const T& arr)
{
    w.startArray();
    for (int i = 0; i < arr.count(); ++i)
         w & arr.at(i);

    return w.endArray();
}

template <typename T>
Reader& Reader::operator& (QList<T>& l)
{
    Reader& r = const_cast<Reader&>(*this);
    return readQArray(r, l);
}

template <typename T>
Writer& Writer::operator& (const QList<T>& l)
{
    Writer& w = const_cast<Writer&>(*this);
    return writeQArray(w, l);
}

template <typename T>
Reader& Reader::operator& (QVector<T>& v)
{
    Reader& r = const_cast<Reader&>(*this);
    return readQArray(r, v);
}

template <typename T>
Writer& Writer::operator& (const QVector<T>& v)
{
    Writer& w = const_cast<Writer&>(*this);
    return writeQArray(w, v);
}

template <typename T>
Reader& Reader::operator& (clife_ptr<T>& ptr)
{
    static_assert(std::is_base_of<clife_base, T>::value,
                  "Class T must be derived from clife_base");

    if (!error())
    {
        if (_stack.top().value->IsNull())
        {
            ptr = clife_ptr<T>();
            Next();
        }
        else if (_stack.top().value->IsObject())
        {
            if (ptr.empty())
                ptr = clife_ptr<T>(new T());
            this->operator& (*ptr);
        }
        else
        {
            setError(1);
            alog::logger().error_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "JSerialize")
                << "Stack top is not object"
                << ". Field: " << stackFieldName()
                << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

template <typename T>
Writer& Writer::operator& (const clife_ptr<T>& ptr)
{
    if (ptr.empty())
    {
        setNull();
        return *this;
    }
    return this->operator& (*ptr);
}

template <int N>
Reader& Reader::operator& (QUuidT<N>& uuid)
{
    return this->operator& (static_cast<QUuid&>(uuid));
}

template <int N>
Writer& Writer::operator& (const QUuidT<N>& uuid)
{
    return this->operator& (static_cast<const QUuid&>(uuid));
}

template<typename T, typename Compare, typename Allocator>
Reader& Reader::operator& (lst::List<T, Compare, Allocator>& list)
{
    Reader& r = const_cast<Reader&>(*this);
    return operatorAmp(r, list);
}

template<typename T, typename Compare, typename Allocator>
Writer& Writer::operator& (const lst::List<T, Compare, Allocator>& list)
{
    startArray();
    for (int i = 0; i < list.count(); ++i)
        this->operator& (list[i]);

    return endArray();
}

//-------------------------------- Functions ---------------------------------

template <typename GenericValueT>
bool stringEqual(const typename GenericValueT::Ch* a, const GenericValueT& b)
{
    RAPIDJSON_ASSERT(b.IsString());

    const SizeType l1 = strlen(a);
    const SizeType l2 = b.GetStringLength();
    if (l1 != l2)
        return false;

    const typename GenericValueT::Ch* const b_ = b.GetString();
    if (a == b_)
        return true;

    return (std::memcmp(a, b_, sizeof(typename GenericValueT::Ch) * l1) == 0);
}

#define J_SERIALIZE_FUNC \
    QByteArray toJson() { \
        serialization::json::Writer writer; \
        jserialize(writer); \
        return QByteArray(writer.getString()); \
    } \
    SResult fromJson(const QByteArray& json) { \
        serialization::json::Reader reader; \
        if (reader.parse(json)) \
            jserialize(reader); \
        return reader.result(); \
    }

/**
  Макросы для работы с функциями сериализации toJson(), fromJson()
*/
#define DECLARE_J_SERIALIZE_FUNC \
    J_SERIALIZE_FUNC \
    template <typename Packer> Packer& jserialize(Packer&);

#define J_SERIALIZE_BEGIN \
    J_SERIALIZE_FUNC \
    template <typename Packer> Packer& jserialize(Packer& p) { \
        p.startObject();

#define J_SERIALIZE_EXTERN_BEGIN(CLASS) \
    template <typename Packer> Packer& CLASS::jserialize(Packer& p) { \
        p.startObject();

#define J_SERIALIZE_ITEM(FIELD) \
        p.member(#FIELD) & FIELD;

#define J_SERIALIZE_OPT(FIELD) \
        p.member(#FIELD, true) & FIELD;

#define J_SERIALIZE_END \
        return p.endObject(); \
    }

#define J_SERIALIZE_BASE_BEGIN \
    template <typename Packer> Packer& jserializeBase(Packer& p) {

#define J_SERIALIZE_BASE_END \
    }

#define J_SERIALIZE_BASE(CLASS) \
    CLASS::jserializeBase(p);

#define J_SERIALIZE_ONE(FIELD) \
    J_SERIALIZE_BEGIN \
    J_SERIALIZE_ITEM(FIELD) \
    J_SERIALIZE_END

#define J_SERIALIZE_BASE_ONE \
    J_SERIALIZE_BEGIN \
    this->jserializeBase(p); \
    J_SERIALIZE_END

} // namespace json
} // namespace serialization
} // namespace communication
