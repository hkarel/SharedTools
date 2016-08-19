#include "message.h"
#include "break_point.h"

namespace communication {

DEFINE_B_SERIALIZE_STREAM_OPERATORS

Message::Ptr Message::create(const QUuidEx& command)
{
    //return Ptr::create_join_ptr(command);
    return Message::Ptr(new Message(command));
}

Message::Message() : _flags(0), _flags2(0)
{
    // Флаги _flags, _flags2 должны быть инициализированы обязательно
    // в конструкторе, так как невозможно корректно выполнить инициализацию
    // не именованных union-параметров при их объявлении в классе.
}

Message::Message(const QUuidEx& command) : Message()
{
    _id = QUuid::createUuid();
    _command = command;
    _commandType = static_cast<quint32>(command::Type::Request);
    _execStatus = Message::ExecStatus::Unknown;
}

BByteArray Message::toByteArray() const
{
    BByteArray ba;
    QDataStream s {&ba, QIODevice::WriteOnly};
    s << *this;
    return std::move(ba);
}

Message::Ptr Message::fromByteArray(const BByteArray& ba)
{
    //Ptr m = Ptr::create_join_ptr();
    Ptr m {new Message()};
    QDataStream s {(BByteArray*)&ba, QIODevice::ReadOnly | QIODevice::Unbuffered};
    s >> *m;
    return std::move(m);
}

bserial::RawVector Message::toRaw() const
{
    B_SERIALIZE_V1(stream, sizeof(Message) * 2 + _content.size())
    stream << _flags;
    stream << _flags2;
    stream << _id;
    stream << _command;
    stream << _maxTimeLife;
    stream << _content;
    B_SERIALIZE_RETURN
}

void Message::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> _flags;
    stream >> _flags2;
    stream >> _id;
    stream >> _command;
    stream >> _maxTimeLife;
    stream >> _content;
    B_DESERIALIZE_END
}



command::Type Message::commandType() const
{
    return static_cast<command::Type>(_commandType);
}

void Message::setCommandType(command::Type val)
{
    _commandType = static_cast<quint32>(val);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

void Message::setExecStatus(ExecStatus val)
{
    _execStatus = val;
}

#pragma GCC diagnostic pop


} // namespace communication
