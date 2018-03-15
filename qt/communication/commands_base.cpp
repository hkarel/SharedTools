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

namespace communication {
namespace command {

#define REGISTRY_COMMAND(COMMAND, UUID) \
    const QUuidEx COMMAND = CommandsPool::Registry{UUID, #COMMAND, true};

REGISTRY_COMMAND(Unknown,            "4aef29d6-5b1a-4323-8655-ef0d4f1bb79d")
REGISTRY_COMMAND(Error,              "b18b98cc-b026-4bfe-8e33-e7afebfbe78b")
REGISTRY_COMMAND(ProtocolCompatible, "173cbbeb-1d81-4e01-bf3c-5d06f9c878c3")
REGISTRY_COMMAND(CloseConnection,    "e71921fd-e5b3-4f9b-8be7-283e8bb2a531")

#undef REGISTRY_COMMAND
} // namespace command

namespace data {

bserial::RawVector MessageError::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << code;
    stream << description;
    B_SERIALIZE_RETURN
}

void MessageError::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> code;
    stream >> description;
    B_DESERIALIZE_END
}

bserial::RawVector MessageFailed::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << code;
    stream << description;
    B_SERIALIZE_RETURN
}

void MessageFailed::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> code;
    stream >> description;
    B_DESERIALIZE_END
}

bserial::RawVector Unknown::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << commandId;
    stream << socketType;
    stream << socketDescriptor;
    stream << socketName;
    stream << address;
    stream << port;
    B_SERIALIZE_RETURN
}

void Unknown::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> commandId;
    stream >> socketType;
    stream >> socketDescriptor;
    stream >> socketName;
    stream >> address;
    stream >> port;
    B_DESERIALIZE_END
}

bserial::RawVector Error::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << commandId;
    stream << code;
    stream << description;
    B_SERIALIZE_RETURN
}

void Error::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> commandId;
    stream >> code;
    stream >> description;
    B_DESERIALIZE_END
}

//bserial::RawVector ProtocolCompatible::toRaw() const
//{
//    B_SERIALIZE_V1(stream)
//    stream << versionLow;
//    stream << versionHigh;
//    B_SERIALIZE_RETURN
//}

//void ProtocolCompatible::fromRaw(const bserial::RawVector& vect)
//{
//    B_DESERIALIZE_V1(vect, stream)
//    stream >> versionLow;
//    stream >> versionHigh;
//    B_DESERIALIZE_END
//}

bserial::RawVector CloseConnection::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << description;
    B_SERIALIZE_RETURN
}

void CloseConnection::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> description;
    B_DESERIALIZE_END
}


} // namespace data
} // namespace communication
