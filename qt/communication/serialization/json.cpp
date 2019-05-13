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
*****************************************************************************/

#include "qt/communication/serialization/json.h"
#include "rapidjson/error/en.h"
#include <atomic>

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "JSerialize")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "JSerialize")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "JSerialize")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "JSerialize")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "JSerialize")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "JSerialize")

namespace communication {
namespace serialization {
namespace json {

#include <cassert>

//#define DOCUMENT reinterpret_cast<Document*>(mDocument)
//#define STACK (reinterpret_cast<ReaderStack*>(mStack))
//#define TOP (STACK->top())
//#define CURRENT (*TOP.value)

//------------------------------- Reader ---------------------------------

static std::atomic<std::uint64_t> jsonIndexReader = {0};

Reader::Reader() : _jsonIndex(jsonIndexReader++)
{}

Reader::~Reader()
{
    if (_hasParseError)
    {
        log_error_m << "Failed parse json"
                    << ". JIndex: " << _jsonIndex
                    << ". Content: " << _jsonContent;
    }
}

SResult Reader::result() const
{
    if (_hasParseError)
    {
        QString msg = "Failed parse json. JIndex: %1";
        SResult res {false, 1, msg.arg(_jsonIndex)};
        return res;
    }
    return SResult(true);
}

bool Reader::parse(const QByteArray& json)
{
    _jsonContent = json;
    _document.Parse(_jsonContent.constData());
    if (_document.HasParseError())
    {
        setError(1);
        ParseErrorCode e = _document.GetParseError();
        int o = int(_document.GetErrorOffset());
        log_error_m << "Failed parse json"
                    << ". JIndex: " << _jsonIndex
                    << ". Error: " << GetParseError_En(e)
                    << " Detail: " << " at offset " << o << " near '"
                    << _jsonContent.mid(o, 20) << "...'";
    }
    else
    {
        setError(0);
        _stack.push({&_document, StackItem::BeforeStart});
    }
    return !error();
}

Reader& Reader::member(const char* name, bool optional)
{
    if (error() < 1)
    {
        if (_stack.top().value->IsObject()
            && _stack.top().state == StackItem::Started)
        {
            Value::ConstMemberIterator memberItr = _stack.top().value->FindMember(name);
            if (memberItr != _stack.top().value->MemberEnd())
            {
                setError(0);
                _stack.push(StackItem{&memberItr->value, StackItem::BeforeStart, name});
            }
            else
            {
                setError(-1, optional);
                if (!optional)
                    log_error_m << "Field '" << name << "' not found"
                                << ". JIndex: " << _jsonIndex;
            }
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not object. JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

//bool Reader::hasMember(const char* name) const
//{
//    if (!error()
//        && _stack.top().value->IsObject()
//        && _stack.top().state == StackItem::Started)
//    {
//        return _stack.top().value->HasMember(name);
//    }
//    return false;
//}

void Reader::setError(int val, bool optional)
{
    _error = val;
    if ((_error != 0) && !optional)
        _hasParseError = true;
}

QByteArray Reader::stackFieldName() const
{
    for (int i = _stack.count() - 1; i >= 0; --i)
        if (!_stack[i].name.isEmpty())
            return _stack[i].name;

    return QByteArray();
}

Reader& Reader::startObject()
{
    if (!error())
    {
        if (_stack.top().value->IsObject()
            && _stack.top().state == StackItem::BeforeStart)
        {
            _stack.top().state = StackItem::Started;
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not object"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::endObject()
{
    if (!error())
    {
        if (_stack.top().value->IsObject()
            && _stack.top().state == StackItem::Started)
        {
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not object"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::startArray(SizeType* size)
{
    if (size)
        *size = 0;

    if (!error())
    {
        if (_stack.top().value->IsArray()
            && _stack.top().state == StackItem::BeforeStart)
        {
            _stack.top().state = StackItem::Started;
            if (size)
                *size = _stack.top().value->Size();

            if (!_stack.top().value->Empty())
            {
                const Value& value = (*_stack.top().value)[_stack.top().index];
                _stack.push(StackItem(&value, StackItem::BeforeStart));
            }
            else
                _stack.top().state = StackItem::Closed;
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not array"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::endArray()
{
    if (!error())
    {
        if (_stack.top().value->IsArray()
            && _stack.top().state == StackItem::Closed)
        {
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not array"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::setNull()
{
    // This function is for Writer only.
    setError(1);
    return *this;
}

Reader& Reader::operator& (bool& b)
{
    if (!error())
    {
        if (_stack.top().value->IsBool())
        {
            b = _stack.top().value->GetBool();
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            b = false;
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not 'bool' type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (qint8& i)
{
    qint32 val;
    this->operator& (val);
    if (!error())
        i = qint8(val);
    return *this;
}

Reader& Reader::operator& (quint8& u)
{
    quint32 val;
    this->operator& (val);
    if (!error())
        u = qint8(val);
    return *this;
}

Reader& Reader::operator& (qint16& i)
{
    qint32 val;
    this->operator& (val);
    if (!error())
        i = qint16(val);
    return *this;
}

Reader& Reader::operator& (quint16& u)
{
    quint32 val;
    this->operator& (val);
    if (!error())
        u = quint16(val);
    return *this;
}

Reader& Reader::operator& (qint32& i)
{
    if (!error())
    {
        if (_stack.top().value->IsInt())
        {
            i = _stack.top().value->GetInt();
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            i = 0;
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not 'int' type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (quint32& u)
{
    if (!error())
    {
        if (_stack.top().value->IsUint())
        {
            u = _stack.top().value->GetUint();
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            u = 0;
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not 'uint' type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (qint64& i)
{
    if (!error())
    {
        if (_stack.top().value->IsInt64())
        {
            i = _stack.top().value->GetInt64();
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            i = 0;
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not int64 type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (quint64& u)
{
    if (!error())
    {
        if (_stack.top().value->IsUint64())
        {
            u = _stack.top().value->GetUint64();
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            u = 0;
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not uint64 type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (double& d)
{
    if (!error())
    {
        if (_stack.top().value->IsNumber())
        {
            d = _stack.top().value->GetDouble();
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            d = 0;
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not number"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (float& f)
{
    double val;
    this->operator& (val);
    if (!error())
        f = float(val);
    return *this;
}

Reader& Reader::operator& (QByteArray& ba)
{
    // Написать реализацию
    break_point

    return *this;
}

Reader& Reader::operator& (QString& s)
{
    if (!error())
    {
        if (_stack.top().value->IsString())
        {
            s = QString::fromUtf8(_stack.top().value->GetString());
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            s = QString();
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not 'string' type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (QDate& date)
{
    if (!error())
    {
        if (_stack.top().value->IsString())
        {
            date = QDate::fromString(_stack.top().value->GetString(), "yyyy-MM-dd");
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            date = QDate();
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not 'string' type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (QTime& time)
{
    if (!error())
    {
        if (_stack.top().value->IsString())
        {
            time = QTime::fromString(_stack.top().value->GetString(), "hh:mm:ss.zzz");
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            time = QTime();
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not 'string' type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (QDateTime& dtime)
{
    if (!error())
    {
        if (_stack.top().value->IsInt64())
        {
            dtime = QDateTime::fromMSecsSinceEpoch(_stack.top().value->GetInt64());
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            dtime = QDateTime();
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not int64 type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

Reader& Reader::operator& (QUuid& uuid)
{
    if (!error())
    {
        if (_stack.top().value->IsString())
        {
            const QByteArray& ba = QByteArray::fromRawData(
                                       _stack.top().value->GetString(),
                                       _stack.top().value->GetStringLength());
            uuid = QUuid(ba);
            Next();
        }
        else if (_stack.top().value->IsNull())
        {
            uuid = QUuid();
            Next();
        }
        else
        {
            setError(1);
            log_error_m << "Stack top is not 'string' type"
                        << ". Field: " << stackFieldName()
                        << ". JIndex: " << _jsonIndex;
        }
    }
    return *this;
}

void Reader::Next()
{
    if (error())
        return;

    assert(!_stack.empty());
    _stack.pop();

    if (!_stack.empty() && _stack.top().value->IsArray())
    {
        // Otherwise means reading array item pass end
        if (_stack.top().state == StackItem::Started)
        {
            if (_stack.top().index < (_stack.top().value->Size() - 1))
            {
                const Value& value = (*_stack.top().value)[++_stack.top().index];
                _stack.push(StackItem(&value, StackItem::BeforeStart));
            }
            else
                _stack.top().state = StackItem::Closed;
        }
        else
        {
            setError(1);
            log_error_m << "Stack top state is not StackItem::Started"
                        << ". JIndex: " << _jsonIndex;
        }
    }
}

//------------------------------- Writer ---------------------------------

Writer::Writer() : _writer(_stream)
{}

Writer::~Writer()
{}

const char* Writer::getString() const
{
    return _stream.GetString();
}

Writer& Writer::member(const char* name, bool)
{
    _writer.String(name, static_cast<SizeType>(strlen(name)));
    return *this;
}

bool Writer::hasMember(const char*) const
{
    // This function is for Reader only.
    assert(false);
    return false;
}

Writer& Writer::startObject()
{
    _writer.StartObject();
    return *this;
}

Writer& Writer::endObject()
{
    _writer.EndObject();
    return *this;
}

Writer& Writer::startArray(SizeType*)
{
    _writer.StartArray();
    return *this;
}

Writer& Writer::endArray()
{
    _writer.EndArray();
    return *this;
}

Writer& Writer::setNull()
{
    _writer.Null();
    return *this;
}

Writer& Writer::operator& (const bool b)
{
    _writer.Bool(b);
    return *this;
}

Writer& Writer::operator& (const qint8 i)
{
    return this->operator& (qint32(i));
}

Writer& Writer::operator& (const quint8 u)
{
    return this->operator& (quint32(u));
}

Writer& Writer::operator& (const qint16 i)
{
    return this->operator& (qint32(i));
}

Writer& Writer::operator& (const quint16 u)
{
    return this->operator& (quint32(u));
}

Writer& Writer::operator& (const qint32 i)
{
    _writer.Int(i);
    return *this;
}

Writer& Writer::operator& (const quint32 u)
{
    _writer.Uint(u);
    return *this;
}

Writer& Writer::operator& (const qint64 i)
{
    _writer.Int64(i);
    return *this;
}

Writer& Writer::operator& (const quint64 u)
{
    _writer.Uint64(u);
    return *this;
}

Writer& Writer::operator& (const double d)
{
    _writer.Double(d);
    return *this;
}

Writer& Writer::operator& (const float f)
{
    _writer.Double(f);
    return *this;
}

Writer& Writer::operator& (const QByteArray& ba)
{
    // Написать реализацию
    break_point

    return *this;
}

Writer& Writer::operator& (const QString& s)
{
    const QByteArray& ba = s.toUtf8();
    _writer.String(ba.constData(), SizeType(ba.length()));
    return *this;
}

Writer& Writer::operator& (const QUuid& uuid)
{
    if (uuid.isNull())
    {
        setNull();
        return *this;
    }

    const QByteArray& ba = uuid.toByteArray();
    if ((ba[0] == '{') && (ba[ba.length()-1] == '}'))
    {
        _writer.String(ba.constData() + 1, SizeType(ba.length() - 2));
    }
    else
    {
        break_point
        // Выяснить в каких случаях нет замыкающих фигурных скобок
        _writer.String(ba.constData(), SizeType(ba.length()));
    }
    return *this;
}

Writer& Writer::operator& (const QDate& date)
{
    if (date.isValid())
    {
        const QByteArray& ba = date.toString("yyyy-MM-dd").toUtf8();
        _writer.String(ba.constData(), SizeType(ba.length()));
    }
    else
        setNull();

    return *this;
}

Writer& Writer::operator& (const QTime& time)
{
    if (time.isValid())
    {
        const QByteArray& ba = time.toString("hh:mm:ss.zzz").toUtf8();
        _writer.String(ba.constData(), SizeType(ba.length()));
    }
    else
        setNull();

    return *this;
}

Writer& Writer::operator& (const QDateTime& dtime)
{
    if (dtime.isValid())
        _writer.Int64(dtime.toMSecsSinceEpoch());
    else
        setNull();

    return *this;
}

} // namespace json
} // namespace serialization
} // namespace communication

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
