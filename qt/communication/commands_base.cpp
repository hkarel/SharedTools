#include "qt/communication/commands_base.h"
#include "qt/communication/commands_pool.h"

namespace communication {
namespace command {

#define REGISTRY_COMMAND(COMMAND, UUID) \
    const QUuidEx COMMAND = CommandsPool::Registry{UUID, #COMMAND};

REGISTRY_COMMAND(Unknown,         "4aef29d6-5b1a-4323-8655-ef0d4f1bb79d")
REGISTRY_COMMAND(Error,           "b18b98cc-b026-4bfe-8e33-e7afebfbe78b")
REGISTRY_COMMAND(CompatibleInfo,  "173cbbeb-1d81-4e01-bf3c-5d06f9c878c3")
REGISTRY_COMMAND(CloseConnection, "e71921fd-e5b3-4f9b-8be7-283e8bb2a531")

#undef REGISTRY_COMMAND
} // namespace command

namespace data {

bserial::RawVector MessageError::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << description;
    B_SERIALIZE_RETURN
}

void MessageError::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> description;
    B_DESERIALIZE_END
}

bserial::RawVector MessageFailed::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << description;
    B_SERIALIZE_RETURN
}

void MessageFailed::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> description;
    B_DESERIALIZE_END
}

bserial::RawVector Unknown::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << commandId;
    stream << address;
    stream << port;
    stream << socketDescriptor;
    B_SERIALIZE_RETURN
}

void Unknown::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> commandId;
    stream >> address;
    stream >> port;
    stream >> socketDescriptor;
    B_DESERIALIZE_END
}

bserial::RawVector Error::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << commandId;
    stream << description;
    B_SERIALIZE_RETURN
}

void Error::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> commandId;
    stream >> description;
    B_DESERIALIZE_END
}

bserial::RawVector CompatibleInfo::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << version.vers;
    stream << minCompatibleVersion.vers;
    stream << commands;
    B_SERIALIZE_RETURN
}

void CompatibleInfo::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> version.vers;
    stream >> minCompatibleVersion.vers;
    stream >> commands;
    B_DESERIALIZE_END
}

bserial::RawVector CompatibleInfo_Answer::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << version.vers;
    stream << minCompatibleVersion.vers;
    stream << commands;
    B_SERIALIZE_RETURN
}

void CompatibleInfo_Answer::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> version.vers;
    stream >> minCompatibleVersion.vers;
    stream >> commands;
    B_DESERIALIZE_END
}

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
