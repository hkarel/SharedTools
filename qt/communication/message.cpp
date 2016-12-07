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
    _commandExecStatus = static_cast<quint32>(command::ExecStatus::Unknown);
    _priority = static_cast<quint32>(Priority::Normal);
    _compression = static_cast<quint32>(Compression::None);
}

void Message::compress(int level, Compression compression)
{
    level = qBound(-1, level, 9);
    if (level != 0
        && _content.size() > 1024 // При меньших значениях компрессирование
                                  // становится не эффективным
        && this->compression() == Compression ::None)
    {
        switch (compression)
        {
            case Compression::Zip:
                _content = qCompress(_content, level);
                _compression = static_cast<quint32>(Compression::Zip);
                break;
            case Compression::Lzma:
                throw std::logic_error("communication::Message: "
                                       "Compression algorithm LZMA not implemented");
            default:
                throw std::logic_error("communication::Message: "
                                       "Unsupported compression algorithm");
        }
    }
}

void Message::decompress()
{
    if (compression() != Compression ::None)
    {
        BByteArray content;
        switch (compression())
        {
            case Compression::Zip:
                content = qUncompress(_content);
                break;
            case Compression::Lzma:
                throw std::logic_error("communication::Message: "
                                       "Compression algorithm LZMA not implemented");
            default:
                throw std::logic_error("communication::Message: "
                                       "Unsupported compression algorithm");
        }
        _content = content;
        _compression = static_cast<quint32>(Compression::None);
    }
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

//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wconversion"

command::ExecStatus Message::commandExecStatus() const
{
    return static_cast<command::ExecStatus>(_commandExecStatus);
}

void Message::setCommandExecStatus(command::ExecStatus val)
{
    _commandExecStatus = static_cast<quint32>(val);
}

Message::Priority Message::priority() const
{
    return static_cast<Priority>(_priority);
}

void Message::setPriority(Priority val)
{
    _priority = static_cast<quint32>(val);
}

Message::Compression Message::compression() const
{
    return static_cast<Compression>(_compression);
}

//#pragma GCC diagnostic pop

} // namespace communication
