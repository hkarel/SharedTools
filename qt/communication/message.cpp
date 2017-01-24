#include "message.h"
#include "break_point.h"
#include "qt/compression/qlzma.h"
#include "qt/compression/qppmd.h"

namespace communication {

DEFINE_B_SERIALIZE_STREAM_OPERATORS

Message::Ptr Message::create(const QUuidEx& command)
{
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
    _type = static_cast<quint32>(Type::Command);
    _execStatus = static_cast<quint32>(ExecStatus::Unknown);
    _priority = static_cast<quint32>(Priority::Normal);
    _compression = static_cast<quint32>(Compression::None);
}

void Message::compress(int level, Compression compression)
{
    if (this->compression() != Compression::None)
        return;

    if (compression == Compression::Disable)
    {
        _compression = static_cast<quint32>(Compression::Disable);
        return;
    }
    level = qBound(-1, level, 9);
    if (level != 0
        && _content.size() > 1024) // При меньших значениях компрессирование
                                   // становится не эффективным
    {
        switch (compression)
        {
            case Compression::Zip:
                _content = qCompress(_content, level);
                _compression = static_cast<quint32>(Compression::Zip);
                break;

            case Compression::Lzma:
            {
                QByteArray content;
                if (qlzma::compress(_content, content, level) == 0)
                {
                    _content = content;
                    _compression = static_cast<quint32>(Compression::Lzma);
                }
                break;
            }
            case Compression::Ppmd:
            {
                QByteArray content;
                if (qppmd::compress(_content, content, level) == 0)
                {
                    _content = content;
                    _compression = static_cast<quint32>(Compression::Ppmd);
                }
                break;
            }
            default:
                throw std::logic_error("communication::Message: "
                                       "Unsupported compression algorithm");
        }
    }
}

void Message::decompress(BByteArray& content) const
{
    switch (compression())
    {
        case Compression::None:
        case Compression::Disable:
            content = _content;
            break;

        case Compression::Zip:
            content = qUncompress(_content);
            break;

        case Compression::Lzma:
            if (qlzma::decompress(_content, content) != 0)
                content.clear();
            break;

        case Compression::Ppmd:
            if (qppmd::decompress(_content, content) != 0)
                content.clear();
            break;

        default:
            throw std::logic_error("communication::Message: "
                                   "Unsupported compression algorithm");
    }
}

void Message::decompress()
{
    if (compression() != Compression::None)
    {
        BByteArray content;
        decompress(content);
        _content = content;
        _compression = static_cast<quint32>(Compression::None);
    }
}

void Message::initEmptyTraits() const
{
    _flags2IsEmpty = (_flags2 == 0);
    _tagIsEmpty = (_tag == 0);
    _maxTimeLifeIsEmpty = (_maxTimeLife == quint64(-1));
    _contentIsEmpty = _content.isEmpty();
}

int Message::size() const
{
    int sz = sizeof(_id)
             + sizeof(_command)
             + sizeof(_protocolVersionLow)
             + sizeof(_protocolVersionHigh)
             + sizeof(_flags);

    initEmptyTraits();

    if (!_flags2IsEmpty)
        sz += sizeof(_flags2);
    if (!_tagIsEmpty)
        sz += sizeof(_tag);
    if (!_maxTimeLifeIsEmpty)
        sz += sizeof(_maxTimeLife);
    if (!_contentIsEmpty)
        sz += _content.size() + sizeof(quint32);

    return sz;
}

BByteArray Message::toByteArray() const
{
    BByteArray ba;
    ba.reserve(size());
    QDataStream stream {&ba, QIODevice::WriteOnly};

    initEmptyTraits();

    stream << _id;
    stream << _command;
    stream << _protocolVersionLow;
    stream << _protocolVersionHigh;
    stream << _flags;

    if (!_flags2IsEmpty)
        stream << _flags2;
    if (!_tagIsEmpty)
        stream << _tag;
    if (!_maxTimeLifeIsEmpty)
        stream << _maxTimeLife;
    if (!_contentIsEmpty)
        stream << _content;

    return std::move(ba);
}

Message::Ptr Message::fromByteArray(const BByteArray& ba)
{
    Ptr m {new Message()};
    QDataStream stream {(BByteArray*)&ba, QIODevice::ReadOnly | QIODevice::Unbuffered};

    stream >> m->_id;
    stream >> m->_command;
    stream >> m->_protocolVersionLow;
    stream >> m->_protocolVersionHigh;
    stream >> m->_flags;

    if (!m->_flags2IsEmpty)
        stream >> m->_flags2;
    if (!m->_tagIsEmpty)
        stream >> m->_tag;
    if (!m->_maxTimeLifeIsEmpty)
        stream >> m->_maxTimeLife;
    if (!m->_contentIsEmpty)
        stream >> m->_content;

    return std::move(m);
}

//bserial::RawVector Message::toRaw() const
//{
//    _tagIsEmpty = (_tag == 0);
//    _maxTimeLifeIsEmpty = (_maxTimeLife == quint64(-1));
//    _contentIsEmpty = _content.isEmpty();
//    _flags2IsEmpty = (_flags2 == 0);

//    B_SERIALIZE_V1(stream, sizeof(Message) * 2 + _content.size())
//    stream << _flags;
//    if (!_flags2IsEmpty)
//        stream << _flags2;
//    stream << _id;
//    stream << _command;
//    if (!_tagIsEmpty)
//        stream << _tag;
//    if (!_maxTimeLifeIsEmpty)
//        stream << _maxTimeLife;
//    if (!_contentIsEmpty)
//        stream << _content;
//    B_SERIALIZE_RETURN
//}

//void Message::fromRaw(const bserial::RawVector& vect)
//{
//    B_DESERIALIZE_V1(vect, stream)
//    stream >> _flags;

//    _flags2 = 0;
//    if (!_flags2IsEmpty)
//        stream >> _flags2;

//    stream >> _id;
//    stream >> _command;

//    _tag = 0;
//    if (!_tagIsEmpty)
//        stream >> _tag;

//    _maxTimeLife = quint64(-1);
//    if (!_maxTimeLifeIsEmpty)
//        stream >> _maxTimeLife;

//    _content.clear();
//    if (!_contentIsEmpty)
//        stream >> _content;
//    B_DESERIALIZE_END
//}

Message::Type Message::type() const
{
    return static_cast<Type>(_type);
}

void Message::setType(Type val)
{
    _type = static_cast<quint32>(val);
}

//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wconversion"

Message::ExecStatus Message::execStatus() const
{
    return static_cast<ExecStatus>(_execStatus);
}

void Message::setExecStatus(ExecStatus val)
{
    _execStatus = static_cast<quint32>(val);
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
