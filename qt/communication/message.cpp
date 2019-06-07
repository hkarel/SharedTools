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

#include "qt/communication/message.h"

#include "break_point.h"
#include "prog_abort.h"
#include "logger/logger.h"
#include "qt/logger/logger_operators.h"

#ifdef LZMA_COMPRESSION
#include "qt/compression/qlzma.h"
#endif
#ifdef PPMD_COMPRESSION
#include "qt/compression/qppmd.h"
#endif

#ifdef JSON_SERIALIZATION
#include "qt/communication/serialization/json.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#endif

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Message")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Message")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Message")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "Message")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Message")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Message")

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
    m->_flag.contentFormat = static_cast<quint32>(SerializationFormat::BProto);

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
    m->_tags = _tags;
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
    int sz = size()
#ifdef UDP_LONGSIG
           + sizeof(quint64); // UDP long signature
#else
           + sizeof(quint32); // UDP signature
#endif

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
                log_error_m << "Unsupported compression algorithm";
                prog_abort();
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
            log_error_m << "Unsupported decompression algorithm";
            prog_abort();
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

    if (_flags2 != 0)
        sz += sizeof(_flags2);

    if (!_tags.isEmpty())
        sz += sizeof(quint8) + _tags.count() * sizeof(quint64);

    if (_maxTimeLife != quint64(-1))
        sz += sizeof(_maxTimeLife);

    if (!_content.isEmpty())
        sz += _content.size() + sizeof(quint32);

    return sz;
}

void Message::initEmptyTraits() const
{
    _flag.flags2IsEmpty      = (_flags2 == 0);
    _flag.tagsIsEmpty        = (_tags.isEmpty());
    _flag.maxTimeLifeIsEmpty = (_maxTimeLife == quint64(-1));
    _flag.contentIsEmpty     = (_content.isEmpty());
}

#ifdef BPROTO_SERIALIZATION
BByteArray Message::toBProto() const
{
    BByteArray ba;
    ba.reserve(size());
    {
        QDataStream stream {&ba, QIODevice::WriteOnly};
        STREAM_INIT(stream);
        toDataStream(stream);
    }
    return std::move(ba);
}

Message::Ptr Message::fromBProto(const BByteArray& ba)
{
    QDataStream stream {(BByteArray*)&ba, QIODevice::ReadOnly | QIODevice::Unbuffered};
    STREAM_INIT(stream);
    return std::move(fromDataStream(stream));
}

void Message::toDataStream(QDataStream& stream) const
{
    initEmptyTraits();

    stream << _id;
    stream << _command;
    stream << _protocolVersionLow;
    stream << _protocolVersionHigh;
    stream << _flags;

    if (!_flag.flags2IsEmpty)
        stream << _flags2;

    if (!_flag.tagsIsEmpty)
    {
        stream << quint8(_tags.size());
        for (quint64 t : _tags)
            stream << t;
    }
    if (!_flag.maxTimeLifeIsEmpty)
        stream << _maxTimeLife;

    if (!_flag.contentIsEmpty)
        stream << _content;
}

Message::Ptr Message::fromDataStream(QDataStream& stream)
{
    Ptr m {new Message};

    stream >> m->_id;
    stream >> m->_command;
    stream >> m->_protocolVersionLow;
    stream >> m->_protocolVersionHigh;
    stream >> m->_flags;

    if (!m->_flag.flags2IsEmpty)
        stream >> m->_flags2;

    if (!m->_flag.tagsIsEmpty)
    {
        quint8 size;
        stream >> size;
        m->_tags.resize(size);
        for (quint8 i = 0; i < size; ++i)
        {
            quint64 t;
            stream >> t;
            m->_tags[i] = t;
        }
    }
    if (!m->_flag.maxTimeLifeIsEmpty)
        stream >> m->_maxTimeLife;

    if (!m->_flag.contentIsEmpty)
        stream >> m->_content;

    return std::move(m);
}
#endif // BPROTO_SERIALIZATION

#ifdef JSON_SERIALIZATION
BByteArray Message::toJson() const
{
    initEmptyTraits();

    using namespace rapidjson;
    StringBuffer buff;
    rapidjson::Writer<StringBuffer> writer(buff);

    writer.StartObject();

    // stream << _id;
    writer.Key("id");
    const QByteArray& id = _id.toByteArray();
    writer.String(id.constData() + 1, SizeType(id.length() - 2));

    // stream << _command;
    writer.Key("command");
    const QByteArray& command = _command.toByteArray();
    writer.String(command.constData() + 1, SizeType(command.length() - 2));

    if (_protocolVersionLow != 0)
    {
        // stream << _protocolVersionLow;
        writer.Key("protocolVersionLow");
        writer.Uint(_protocolVersionLow);
    }
    if (_protocolVersionHigh != 0)
    {
        // stream << _protocolVersionHigh;
        writer.Key("protocolVersionHigh");
        writer.Uint(_protocolVersionHigh);
    }

    // stream << _flags;
    writer.Key("flags");
    writer.Uint(_flags);

    if (!_flag.flags2IsEmpty)
    {
        // stream << _flags2;
        writer.Key("flags2");
        writer.Uint(_flags2);
    }
    if (!_flag.tagsIsEmpty)
    {
        writer.Key("tags");
        writer.StartArray();
        for (int i = 0; i < _tags.count(); ++i)
            writer.Uint64(_tags.at(i));
        writer.EndArray();
    }
    if (!_flag.maxTimeLifeIsEmpty)
    {
        // stream << _maxTimeLife;
        writer.Key("maxTimeLife");
        writer.Uint64(_maxTimeLife);
    }
    if (!_flag.contentIsEmpty)
    {
        // stream << _content;
        writer.Key("content");
        writer.RawValue(_content.constData(), size_t(_content.length()), kObjectType);
    }
    writer.EndObject();
    return BByteArray(buff.GetString());
}

Message::Ptr Message::fromJson(const BByteArray& ba)
{
    Ptr m {new Message};

    using namespace rapidjson;
    using namespace serialization::json;
    Document doc;
    doc.Parse(ba.constData(), size_t(ba.length()));

    if (doc.HasParseError())
    {
        ParseErrorCode e = doc.GetParseError();
        int o = int(doc.GetErrorOffset());
        log_error_m << "Failed parce json."
                    << " Error: " << GetParseError_En(e)
                    << " Detail: " << " at offset " << o << " near '"
                    << ba.mid(o, 20) << "...'";
        return std::move(m);
    }
    if (!doc.IsObject())
    {
        log_error_m << "Failed json format";
        return std::move(m);
    }

    for (auto member = doc.MemberBegin(); member != doc.MemberEnd(); ++member)
    {
        if (stringEqual("id", member->name) && member->value.IsString())
        {
            const QByteArray& ba = QByteArray::fromRawData(member->value.GetString(),
                                                           member->value.GetStringLength());
            m->_id = QUuidEx(ba);
        }
        else if (stringEqual("command", member->name) && member->value.IsString())
        {
            const QByteArray& ba = QByteArray::fromRawData(member->value.GetString(),
                                                           member->value.GetStringLength());
            m->_command = QUuidEx(ba);
        }
        else if (stringEqual("protocolVersionLow", member->name) && member->value.IsUint())
        {
            m->_protocolVersionLow = quint16(member->value.GetUint());
        }
        else if (stringEqual("protocolVersionHigh", member->name) && member->value.IsUint())
        {
            m->_protocolVersionHigh = quint16(member->value.GetUint());
        }
        else if (stringEqual("flags", member->name) && member->value.IsUint())
        {
            m->_flags = quint32(member->value.GetUint());
        }
        else if (stringEqual("flags2", member->name) && member->value.IsUint())
        {
            m->_flags2 = quint32(member->value.GetUint());
        }
        else if (stringEqual("tags", member->name) && member->value.IsArray())
        {
            int size = int(member->value.Size());
            m->_tags.resize(size);
            for (int i = 0; i < size; ++i)
                m->_tags[i] = member->value[SizeType(i)].GetUint64();
        }
        else if (stringEqual("maxTimeLife", member->name) && member->value.IsUint64())
        {
            m->_maxTimeLife = quint64(member->value.GetUint64());
        }
        else if (stringEqual("content", member->name) && member->value.IsObject())
        {
            StringBuffer buff;
            rapidjson::Writer<StringBuffer> writer {buff};
            member->value.Accept(writer);
            m->_content = QByteArray(buff.GetString());
        }
    }
    return std::move(m);
}
#endif // JSON_SERIALIZATION

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

quint64 Message::tag(int index) const
{
    if (!lst::inRange(index, 0, 255))
    {
        log_error_m << "Index value not in range [0..254]";
        return 0;
    }
    if (index >= _tags.count())
        return 0;

    return _tags[index];
}

void Message::setTag(quint64 val, int index)
{
    if (!lst::inRange(index, 0, 255))
    {
        log_error_m << "Index value not in range [0..254]";
        return;
    }
    if (index >= _tags.count())
        _tags.resize(index + 1);

    _tags[index] = val;
}

void Message::setTags(const QVector<quint64>& val)
{
    _tags = val;
    if (_tags.count() > 255)
    {
        log_error_m << "Size of tags array great then 255"
                       ". Array will be truncated to 255";
        _tags.resize(255);
    }
}

Message::Compression Message::compression() const
{
    return static_cast<Compression>(_flag.compression);
}

SerializationFormat Message::contentFormat() const
{
    return static_cast<SerializationFormat>(_flag.contentFormat);
}

void Message::setContentFormat(SerializationFormat val)
{
    _flag.contentFormat = static_cast<quint32>(val);
}

} // namespace communication

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
