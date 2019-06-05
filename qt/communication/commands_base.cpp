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
*****************************************************************************/

#include "qt/communication/commands_base.h"
#include "qt/communication/commands_pool.h"
#include "qt/logger/logger_operators.h"

namespace communication {
namespace command {

#define REGISTRY_COMMAND(COMMAND, UUID) \
    const QUuidEx COMMAND = command::Pool::Registry{UUID, #COMMAND, true};

REGISTRY_COMMAND(Unknown,            "4aef29d6-5b1a-4323-8655-ef0d4f1bb79d")
REGISTRY_COMMAND(Error,              "b18b98cc-b026-4bfe-8e33-e7afebfbe78b")
REGISTRY_COMMAND(ProtocolCompatible, "173cbbeb-1d81-4e01-bf3c-5d06f9c878c3")
REGISTRY_COMMAND(CloseConnection,    "e71921fd-e5b3-4f9b-8be7-283e8bb2a531")

#undef REGISTRY_COMMAND
} // namespace command

namespace data {

MessageError::MessageError(qint32 group, const QUuidEx& code, const QString& description)
    : group(group), code(code), description(description)
{}

MessageError::MessageError(qint32 group, const QUuidEx& code, const char* description)
    : MessageError(group, code, QString::fromUtf8(description))
{}

bserial::RawVector MessageError::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << group;
    stream << code;
    /* stream << description */
    B_QSTR_TO_UTF8(stream, description);
    B_SERIALIZE_RETURN
}

void MessageError::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> group;
    stream >> code;
    /* stream >> description */
    B_QSTR_FROM_UTF8(stream, description);
    B_DESERIALIZE_END
}

MessageFailed::MessageFailed(qint32 group, const QUuidEx& code, const QString& description)
    : group(group), code(code), description(description)
{}

MessageFailed::MessageFailed(qint32 group, const QUuidEx& code, const char* description)
    : MessageFailed(group, code, QString::fromUtf8(description))
{}

bserial::RawVector MessageFailed::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << group;
    stream << code;
    /* stream << description */
    B_QSTR_TO_UTF8(stream, description);
    B_SERIALIZE_RETURN
}

void MessageFailed::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> group;
    stream >> code;
    /* stream >> description */
    B_QSTR_FROM_UTF8(stream, description);
    B_DESERIALIZE_END
}

bserial::RawVector Unknown::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << commandId;
    stream << socketType;
    stream << socketDescriptor;
    /* stream << socketName */
    B_QSTR_TO_UTF8(stream, socketName);

    // Отладить: stream << address
    break_point

    /* stream << address
       Реализация взята с QHostAddress
    */
    switch (int(address.protocol()))
    {
        case QAbstractSocket::IPv4Protocol:
        {
            stream << quint8(0); // protocol
            stream << address.toIPv4Address();
            break;
        }
        case QAbstractSocket::IPv6Protocol:
        {
            stream << quint8(1); // protocol
            Q_IPV6ADDR ipv6 = address.toIPv6Address();
            for (int i = 0; i < 16; ++i)
                stream << ipv6[i];

            /* stream << address.scopeId() */
            B_QSTR_TO_UTF8(stream, address.scopeId());
            break;
        }
    }
    stream << port;
    B_SERIALIZE_RETURN
}

void Unknown::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> commandId;
    stream >> socketType;
    stream >> socketDescriptor;
    /* stream >> socketName */
    B_QSTR_FROM_UTF8(stream, socketName);

    // Отладить: stream >> address
    break_point

    /* stream >> address
       Реализация взята с QHostAddress
    */
    address.clear();
    qint8 prot;
    stream >> prot;
    switch (prot)
    {
        case 0: /* QAbstractSocket::IPv4Protocol */
        {
            quint32 ipv4;
            stream >> ipv4;
            address.setAddress(ipv4);
            break;
        }
        case 1: /* QAbstractSocket::IPv6Protocol */
        {
            Q_IPV6ADDR ipv6;
            for (int i = 0; i < 16; ++i)
                stream >> ipv6[i];
            address.setAddress(ipv6);

            QString scope;
            /* stream >> scope */
            B_QSTR_FROM_UTF8(stream, scope);
            address.setScopeId(scope);
            break;
        }
    }
    stream >> port;
    B_DESERIALIZE_END
}

bserial::RawVector Error::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << commandId;
    stream << messageId;
    stream << group;
    stream << code;
    /* stream << description */
    B_QSTR_TO_UTF8(stream, description);
    B_SERIALIZE_RETURN
}

void Error::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> commandId;
    stream >> messageId;
    stream >> group;
    stream >> code;
    /* stream >> description */
    B_QSTR_FROM_UTF8(stream, description);
    B_DESERIALIZE_END
}

bserial::RawVector CloseConnection::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << code;
    /* stream << description */
    B_QSTR_TO_UTF8(stream, description);
    B_SERIALIZE_RETURN
}

void CloseConnection::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> code;
    /* stream >> description */
    B_QSTR_FROM_UTF8(stream, description);
    B_DESERIALIZE_END
}

} // namespace data

namespace error {

QHashEx<QUuidEx, int>& pool()
{
    static QHashEx<QUuidEx, int> p;
    return p;
}

bool checkUnique()
{
    auto p = pool();
    for (auto it = p.constBegin(); it != p.constEnd(); ++it)
        if (it.value() != 1)
        {
            log_error << "Not unique error code: " << it.key();
            return false;
        }

    pool().clear();
    return true;
}

#define REGISTRY_GERROR(COMMAND, UUID) \
    const QUuidEx COMMAND = command::Pool::Registry{UUID, "error_"#COMMAND, true};

REGISTRY_GERROR(MessageContentParse,  "d603db4a-bf1a-4a55-8df7-ab667684bf3e")

#undef REGISTRY_GERROR

} // namespace error
} // namespace communication
