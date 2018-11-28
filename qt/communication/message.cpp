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

#include "break_point.h"
#ifdef LZMA_COMPRESSION
#include "qt/compression/qlzma.h"
#endif
#ifdef PPMD_COMPRESSION
#include "qt/compression/qppmd.h"
#endif
#include "qt/communication/message.h"
#include <stdexcept>

namespace communication {

Message::Message() : _flags(0), _flags2(0)
{
    // Флаги _flags, _flags2 должны быть инициализированы обязательно
    // в конструкторе, так как невозможно корректно выполнить инициализацию
    // не именованных union-параметров при их объявлении в классе.
}

Message::Ptr Message::create(const QUuidEx& command)
{
    Ptr m {new Message};

    m->_id = QUuid::createUuid();
    m->_command = command;
    m->_flag.type = static_cast<quint32>(Type::Command);
    m->_flag.execStatus = static_cast<quint32>(ExecStatus::Unknown);
    m->_flag.priority = static_cast<quint32>(Priority::Normal);
    m->_flag.compression = static_cast<quint32>(Compression::None);

    return std::move(m);
}

Message::Ptr Message::cloneForAnswer() const
{
    Ptr m {new Message};

    // Клонируемые параметры
    m->_id = _id;
    m->_command = _command;
    m->_protocolVersionLow = _protocolVersionLow;
    m->_protocolVersionHigh = _protocolVersionHigh;
    m->_flags = _flags;
    m->_flags2 = _flags2;
    m->_tag = _tag;
    m->_maxTimeLife = _maxTimeLife;
    m->_socketType = _socketType;
    m->_sourcePoint = _sourcePoint;
    m->_socketDescriptor = _socketDescriptor;
    m->_socketName = _socketName;
    m->_auxiliary = _auxiliary;

    // Инициализируемые параметры
    m->_flag.type = static_cast<quint32>(Type::Answer);
    m->_flag.execStatus = static_cast<quint32>(ExecStatus::Success);
    m->_flag.compression = static_cast<quint32>(Compression::None);

    return std::move(m);
}

SocketDescriptorSet& Message::destinationSocketDescriptors()
{
    return _destinationSocketDescriptors;
}

void Message::compress(int level, Compression compression)
{
    if (this->compression() != Compression::None)
        return;

    if (compression == Compression::Disable)
    {
        _flag.compression = static_cast<quint32>(Compression::Disable);
        return;
    }
    level = qBound(-1, level, 9);
    int sz = size() + sizeof(quint32 /*UDP signature*/);

    // Здесь 508 это минимальный размер UDP пакета передаваемого по сети без
    // фрагментации. Уже при значении 508 сжатие становится мало эффективным,
    // но мы все равно пытаемся сжать пакет, чтобы уложиться в границы 508-ми
    // байт.
    if (level != 0 && sz > 508)
    {
        switch (compression)
        {
            case Compression::Zip:
                _content = qCompress(_content, level);
                _flag.compression = static_cast<quint32>(Compression::Zip);
                break;
#ifdef LZMA_COMPRESSION
            case Compression::Lzma:
            {
                QByteArray content;
                if (qlzma::compress(_content, content, level) == 0)
                {
                    _content = content;
                    _flag.compression = static_cast<quint32>(Compression::Lzma);
                }
                break;
            }
#endif
#ifdef PPMD_COMPRESSION
            case Compression::Ppmd:
            {
                QByteArray content;
                if (qppmd::compress(_content, content, level) == 0)
                {
                    _content = content;
                    _flag.compression = static_cast<quint32>(Compression::Ppmd);
                }
                break;
            }
#endif
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
#ifdef LZMA_COMPRESSION
        case Compression::Lzma:
            if (qlzma::decompress(_content, content) != 0)
                content.clear();
            break;
#endif
#ifdef PPMD_COMPRESSION
        case Compression::Ppmd:
            if (qppmd::decompress(_content, content) != 0)
                content.clear();
            break;
#endif
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
        _flag.compression = static_cast<quint32>(Compression::None);
    }
}

int Message::size() const
{
    int sz = sizeof(_id)
             + sizeof(_command)
             + sizeof(_protocolVersionLow)
             + sizeof(_protocolVersionHigh)
             + sizeof(_flags);

    if (_flags2 != 0)                sz += sizeof(_flags2);
    if (_tag != 0)                   sz += sizeof(_tag);
    if (_maxTimeLife != quint64(-1)) sz += sizeof(_maxTimeLife);
    if (!_content.isEmpty())         sz += _content.size() + sizeof(quint32);

    return sz;
}

BByteArray Message::toByteArray() const
{
    BByteArray ba;
    ba.reserve(size());
    {
        QDataStream stream {&ba, QIODevice::WriteOnly};
        stream.setByteOrder(QDATASTREAM_BYTEORDER);
        stream.setVersion(QDATASTREAM_VERSION);
        toDataStream(stream);
    }
    return std::move(ba);
}

void Message::toDataStream(QDataStream& stream) const
{
    _flag.flags2IsEmpty      = (_flags2 == 0);
    _flag.tagIsEmpty         = (_tag == 0);
    _flag.maxTimeLifeIsEmpty = (_maxTimeLife == quint64(-1));
    _flag.contentIsEmpty     = _content.isEmpty();

    stream << _id;
    stream << _command;
    stream << _protocolVersionLow;
    stream << _protocolVersionHigh;
    stream << _flags;

    if (!_flag.flags2IsEmpty)      stream << _flags2;
    if (!_flag.tagIsEmpty)         stream << _tag;
    if (!_flag.maxTimeLifeIsEmpty) stream << _maxTimeLife;
    if (!_flag.contentIsEmpty)     stream << _content;
}

Message::Ptr Message::fromByteArray(const BByteArray& ba)
{
    QDataStream stream {(BByteArray*)&ba, QIODevice::ReadOnly | QIODevice::Unbuffered};
    stream.setByteOrder(QDATASTREAM_BYTEORDER);
    stream.setVersion(QDATASTREAM_VERSION);
    return std::move(fromDataStream(stream));
}

Message::Ptr Message::fromDataStream(QDataStream& stream)
{
    Ptr m {new Message};

    stream >> m->_id;
    stream >> m->_command;
    stream >> m->_protocolVersionLow;
    stream >> m->_protocolVersionHigh;
    stream >> m->_flags;

    if (!m->_flag.flags2IsEmpty)      stream >> m->_flags2;
    if (!m->_flag.tagIsEmpty)         stream >> m->_tag;
    if (!m->_flag.maxTimeLifeIsEmpty) stream >> m->_maxTimeLife;
    if (!m->_flag.contentIsEmpty)     stream >> m->_content;

    return std::move(m);
}

Message::Type Message::type() const
{
    return static_cast<Type>(_flag.type);
}

void Message::setType(Type val)
{
    _flag.type = static_cast<quint32>(val);
}

Message::ExecStatus Message::execStatus() const
{
    return static_cast<ExecStatus>(_flag.execStatus);
}

void Message::setExecStatus(ExecStatus val)
{
    _flag.execStatus = static_cast<quint32>(val);
}

Message::Priority Message::priority() const
{
    return static_cast<Priority>(_flag.priority);
}

void Message::setPriority(Priority val)
{
    _flag.priority = static_cast<quint32>(val);
}

Message::Compression Message::compression() const
{
    return static_cast<Compression>(_flag.compression);
}

} // namespace communication
