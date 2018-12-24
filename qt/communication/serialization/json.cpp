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
#include "logger/logger.h"
#include "qt/logger/logger_operators.h"
#include "rapidjson/error/en.h"

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

Reader::Reader()
{}

Reader::~Reader()
{}

bool Reader::parse(const QByteArray& json)
{
    _document.Parse(json.constData());
    if (_document.HasParseError())
    {
        _error = true;
        ParseErrorCode e = _document.GetParseError();
        int o = int(_document.GetErrorOffset());
        log_error_m << "Failed parce json."
                    << " Error: " << GetParseError_En(e)
                    << " Detail: " << " at offset " << o << " near '"
                    << json.mid(o, 20) << "...'";
    }
    else
    {
        _error = false;
        _stack.push({&_document, StackItem::BeforeStart});
    }
    return !_error;
}

Reader& Reader::member(const char* name)
{
    if (!_error)
    {
        if (_stack.top().value->IsObject()
            && _stack.top().state == StackItem::Started)
        {
            Value::ConstMemberIterator memberItr = _stack.top().value->FindMember(name);
            if (memberItr != _stack.top().value->MemberEnd())
            {
                _stack.push(StackItem(&memberItr->value, StackItem::BeforeStart));
            }
            else
                _error = true;
        }
        else
            _error = true;
    }
    return *this;
}

bool Reader::hasMember(const char* name) const
{
    if (!_error
        && _stack.top().value->IsObject()
        && _stack.top().state == StackItem::Started)
    {
        return _stack.top().value->HasMember(name);
    }
    return false;
}

Reader& Reader::startObject()
{
    if (!_error)
    {
        if (_stack.top().value->IsObject()
            && _stack.top().state == StackItem::BeforeStart)
        {
            _stack.top().state = StackItem::Started;
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::endObject()
{
    if (!_error)
    {
        if (_stack.top().value->IsObject()
            && _stack.top().state == StackItem::Started)
        {
            Next();
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::startArray(SizeType* size)
{
    if (!_error)
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
            _error = true;
    }
    return *this;
}

Reader& Reader::endArray()
{
    if (!_error)
    {
        if (_stack.top().value->IsArray()
            && _stack.top().state == StackItem::Closed)
        {
            Next();
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::setNull()
{
    // This function is for Writer only.
    _error = true;
    return *this;
}

Reader& Reader::operator& (bool& b)
{
    if (!_error)
    {
        if (_stack.top().value->IsBool())
        {
            b = _stack.top().value->GetBool();
            Next();
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::operator& (qint8& i)
{
    qint32 val;
    this->operator& (val);
    if (!hasParseError())
        i = qint8(val);
    return *this;
}

Reader& Reader::operator& (quint8& u)
{
    quint32 val;
    this->operator& (val);
    if (!hasParseError())
        u = qint8(val);
    return *this;
}

Reader& Reader::operator& (qint16& i)
{
    qint32 val;
    this->operator& (val);
    if (!hasParseError())
        i = qint16(val);
    return *this;
}

Reader& Reader::operator& (quint16& u)
{
    quint32 val;
    this->operator& (val);
    if (!hasParseError())
        u = quint16(val);
    return *this;
}

Reader& Reader::operator& (qint32& i)
{
    if (!_error)
    {
        if (_stack.top().value->IsInt())
        {
            i = _stack.top().value->GetInt();
            Next();
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::operator& (quint32& u)
{
    if (!_error)
    {
        if (_stack.top().value->IsUint())
        {
            u = _stack.top().value->GetUint();
            Next();
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::operator& (qint64& i)
{
    if (!_error)
    {
        if (_stack.top().value->IsInt64())
        {
            i = _stack.top().value->GetInt64();
            Next();
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::operator& (quint64& u)
{
    if (!_error)
    {
        if (_stack.top().value->IsUint64())
        {
            u = _stack.top().value->GetUint64();
            Next();
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::operator& (double& d)
{
    if (!_error)
    {
        if (_stack.top().value->IsNumber())
        {
            d = _stack.top().value->GetDouble();
            Next();
        }
        else
            _error = true;
    }
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
    if (!_error)
    {
        if (_stack.top().value->IsString())
        {
            s = QString::fromUtf8(_stack.top().value->GetString());
            Next();
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::operator& (QUuid& uuid)
{
    if (!_error)
    {
        if (_stack.top().value->IsString())
        {
            const QByteArray& ba = QByteArray::fromRawData(_stack.top().value->GetString(),
                                                           _stack.top().value->GetStringLength());
            uuid = QUuid(ba);
            Next();
        }
        else
            _error = true;
    }
    return *this;
}

Reader& Reader::operator& (QUuidEx& uuid)
{
    return this->operator& (static_cast<QUuid&>(uuid));
}

void Reader::Next()
{
    if (!_error)
    {
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
                _error = true;
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

Writer& Writer::member(const char* name)
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
    const QByteArray& ba = uuid.toByteArray();
    _writer.String(ba.constData(), SizeType(ba.length()));
    return *this;
}

Writer& Writer::operator& (const QUuidEx& uuid)
{
    return this->operator& (static_cast<const QUuid&>(uuid));
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
