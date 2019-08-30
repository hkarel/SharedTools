/*****************************************************************************
  The MIT License

  Copyright © 2017 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "qt/communication/transport/base.h"

#include "break_point.h"
#include "prog_abort.h"
#include "spin_locker.h"
#include "logger/logger.h"
#include "qt/logger/logger_operators.h"
#include "qt/version/version_number.h"
#include "qt/communication/commands_pool.h"
#include "qt/communication/logger_operators.h"

#include <utility>
#include <stdexcept>

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")

namespace communication {
namespace transport {
namespace base {

//--------------------------------- Base -------------------------------------

void Properties::setCompressionLevel(int val)
{
    _compressionLevel = qBound(-1, val, 9);
}

//----------------------------- SocketCommon ---------------------------------

bool SocketCommon::send(const Message::Ptr& message)
{
    if (!isRunning())
    {
        log_error_m << "Socket is not active"
                    << ". Command " << CommandNameLog(message->command())
                    << " will be discarded";
        return false;
    }
    if (message.empty())
    {
        log_error_m << "Cannot send empty message";
        return false;
    }
    if (_checkUnknownCommands)
    {
        bool isUnknown;
        { //Block for SpinLocker
            SpinLocker locker(_unknownCommandsLock); (void) locker;
            isUnknown = _unknownCommands.contains(message->command());
        }
        if (isUnknown)
        {
            log_error_m << "Command " << CommandNameLog(message->command())
                        << " is unknown for receiving side"
                        << ". Command will be discarded";
            return false;
        }
    }
    message->add_ref();
    { //Block for QMutexLocker
        QMutexLocker locker(&_messagesLock); (void) locker;
        switch (message->priority())
        {
            case Message::Priority::High:
                _messagesHigh.add(message.get());
                break;
            case Message::Priority::Low:
                _messagesLow.add(message.get());
                break;
            default:
                _messagesNorm.add(message.get());
        }

        if (alog::logger().level() == alog::Level::Debug2)
        {
            // Это сообщение нужно добавлять в лог до вызова _messagesCond.wakeAll(),
            // иначе в логе может возникнуть путаница с порядком следования сообщений
            log_debug2_m << "Message added to queue to sending"
                         << ". Id: " << message->id()
                         << ". Command: " << CommandNameLog(message->command());
        }
        _messagesCond.wakeAll();
    }
    return true;
}

void SocketCommon::remove(const QUuidEx& command)
{
    QMutexLocker locker(&_messagesLock); (void) locker;
    auto funcCond = [&command](Message* m) -> bool
    {
        bool res = (command == m->command());
        if (res && (alog::logger().level() == alog::Level::Debug2))
            log_debug2_m << "Message removed from queue to sending"
                         << ". Id: " << m->id()
                         << ". Command: " << CommandNameLog(m->command());
        return res;
    };
    _messagesHigh.removeCond(funcCond);
    _messagesNorm.removeCond(funcCond);
    _messagesLow .removeCond(funcCond);
}

int SocketCommon::messagesCount() const
{
    QMutexLocker locker(&_messagesLock); (void) locker;
    return _messagesHigh.count()
           + _messagesNorm.count()
           + _messagesLow.count();
}

//-------------------------------- Socket ------------------------------------

Socket::Socket(SocketType type) : _type(type)
{
    registrationQtMetatypes();

#ifdef BPROTO_SERIALIZATION
    _protocolMap << qMakePair(SerializationFormat::BProto, QUuidEx{"82c40273-4037-4f1b-a823-38123435b22f"});
#endif
#ifdef JSON_SERIALIZATION
    _protocolMap << qMakePair(SerializationFormat::Json,   QUuidEx{"fea6b958-dafb-4f5c-b620-fe0aafbd47e2"});
#endif
}

bool Socket::isConnected() const
{
    return (socketIsConnected()
            && _protocolCompatible == ProtocolCompatible::Yes);
}

bool Socket::socketIsConnected() const
{
    SpinLocker locker(_socketLock); (void) locker;
    return socketIsConnectedInternal();
}

bool Socket::isLocal() const
{
    SpinLocker locker(_socketLock); (void) locker;
    return isLocalInternal();
}

Socket::ProtocolCompatible Socket::protocolCompatible() const
{
    return _protocolCompatible;
}

SocketDescriptor Socket::socketDescriptor() const
{
    SpinLocker locker(_socketLock); (void) locker;
    return socketDescriptorInternal();
}

void Socket::connect()
{
    start();
}

void Socket::disconnect(unsigned long time)
{
    stop(time);
}

void Socket::socketDisconnected()
{
    _serializationSignatureRead = false;
    _serializationSignatureWrite = false;

    emit disconnected(_initSocketDescriptor);
    _initSocketDescriptor = {-1};
}

void Socket::waitConnection(int time)
{
    if ((time <= 0) || isConnected())
        return;

    QElapsedTimer timer;
    timer.start();
    while (!timer.hasExpired(time * 1000))
    {
        if (threadStop())
            break;

        msleep(100);
        if (isConnected())
            break;
    }
}

void Socket::setMessageFormat(SerializationFormat val)
{
    if (socketIsConnected() || isListenerSide())
        return;

    _messageFormat = val;
}

void Socket::run()
{
    { // Block for SpinLocker
        SpinLocker locker(_socketLock); (void) locker;
        socketCreate();
    }

    if (!socketInit())
    {
        SpinLocker locker(_socketLock); (void) locker;
        socketClose();
        _initSocketDescriptor = {-1};
        return;
    }
    _initSocketDescriptor = socketDescriptorInternal();

    Message::List internalMessages;
    Message::List acceptMessages;

    QElapsedTimer timer;
    bool loopBreak = false;
    const int delay = 50;

    BByteArray readBuff;
    qint32 readBuffSize = 0;
    char* readBuffCur = 0;
    char* readBuffEnd = 0;

    // Сигнатура формата сериализации
    QUuidEx serializationSignature;
    for (const auto& sign : _protocolMap)
        if (sign.first == _messageFormat)
        {
            serializationSignature = sign.second;
            break;
        }

    if (!isListenerSide() && serializationSignature.isNull())
    {
        log_error_m << "Message serialization format signature undefined";
        SpinLocker locker(_socketLock); (void) locker;
        socketClose();
        prog_abort();
    }

    // Идентификатор команды CloseConnection, используется для отслеживания
    // ответа на запрос о разрыве соединения.
    QUuidEx commandCloseConnectionId;

    _protocolCompatible = ProtocolCompatible::Undefined;
    {
        // Добавляем самое первое сообщение с информацией о совместимости
        Message::Ptr m = Message::create(command::ProtocolCompatible, _messageFormat);
        m->setPriority(Message::Priority::High);
        internalMessages.add(m.detach());
    }

    auto processingProtocolCompatibleCommand = [&](Message::Ptr& message) -> void
    {
        if (message->command() != command::ProtocolCompatible)
            return;

        if (message->type() == Message::Type::Command)
        {
            quint16 protocolVersionLow  = message->protocolVersionLow();
            quint16 protocolVersionHigh = message->protocolVersionHigh();

            _protocolCompatible = ProtocolCompatible::Yes;
            if (_checkProtocolCompatibility)
            {
                log_debug_m << "Checking protocol compatibility"
                            << ". This protocol version: "
                            << BPROTOCOL_VERSION_LOW << "-" << BPROTOCOL_VERSION_HIGH
                            << ". Remote protocol version: "
                            << protocolVersionLow << "-" << protocolVersionHigh;

                if (!communication::protocolCompatible(protocolVersionLow, protocolVersionHigh))
                    _protocolCompatible = ProtocolCompatible::No;
            }

            if (_protocolCompatible == ProtocolCompatible::Yes)
            {
                if (isListenerSide())
                    while (!isInsideListener()) {msleep(10);}

                emit connected(socketDescriptorInternal());
            }
            else // ProtocolCompatible::No
            {
                data::CloseConnection closeConnection;
                closeConnection.code = 0;
                closeConnection.description = QString(
                    "Protocol versions incompatible"
                    ". This protocol version: %1-%2"
                    ". Remote protocol version: %3-%4"
                )
                // Инвертируем версии протокола, иначе на принимающей стороне
                // будет путаница с восприятием.
                .arg(protocolVersionLow).arg(protocolVersionHigh)
                .arg(BPROTOCOL_VERSION_LOW).arg(BPROTOCOL_VERSION_HIGH);

                log_verbose_m << "Send request to close connection"
                              << ". Detail: " << closeConnection.description;

                Message::Ptr m = createMessage(closeConnection, {_messageFormat});
                m->setPriority(Message::Priority::High);
                internalMessages.add(m.detach());
            }
        }
    };

    auto processingCloseConnectionCommand = [&](Message::Ptr& message) -> void
    {
        if (message->command() != command::CloseConnection)
            return;

        if (message->type() == Message::Type::Command)
        {
            data::CloseConnection closeConnection;
            readFromMessage(message, closeConnection);
            if (closeConnection.dataIsValid)
                log_verbose_m << "Connection will be closed at request remote side"
                              << ". Remote detail: " << closeConnection.description;
            else
                log_error_m << "Incorrect data structure for command "
                            << CommandNameLog(message->command());

            // Отправляем ответ
            Message::Ptr answer = message->cloneForAnswer();
            answer->setPriority(Message::Priority::High);
            internalMessages.add(answer.detach());
        }
        else if (message->type() == Message::Type::Answer
                 && message->id() == commandCloseConnectionId)
        {
            loopBreak = true;
        }
    };

    #define CHECK_SOCKET_ERROR \
        if (!socketIsConnectedInternal()) \
        { \
            printSocketError(__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport"); \
            loopBreak = true; \
            break; \
        }

    try
    {
        while (true)
        {
            if (threadStop())
                break;

            CHECK_SOCKET_ERROR
            if (loopBreak)
                break;

            // Отправка сигнатуры протокола
            if (!_serializationSignatureWrite && !isListenerSide())
            {
                QByteArray ba;
                QDataStream stream {&ba, QIODevice::WriteOnly};
                STREAM_INIT(stream);
                stream << serializationSignature;
                socketWrite(ba.constData(), 16);
                CHECK_SOCKET_ERROR
                socketWaitForBytesWritten(delay);
                CHECK_SOCKET_ERROR

                alog::Line logLine =
                    log_verbose_m << "Message serialization format: ";
                switch (_messageFormat)
                {
#ifdef BPROTO_SERIALIZATION
                    case SerializationFormat::BProto:
                        logLine << "bproto";
                        break;
#endif
#ifdef JSON_SERIALIZATION
                    case SerializationFormat::Json:
                        logLine << "json";
                        break;
#endif
                    default:
                        logLine << "unknown";
                }
                _serializationSignatureWrite = true;
            }

            // Проверка сигнатуры протокола
            if (!_serializationSignatureRead)
            {
                timer.start();
                int timeout;
                if (isListenerSide())
                    timeout = 60  * delay;  // 3 сек
                else
                    timeout = 120 * delay;  // 6 сек

                while (socketBytesAvailable() < 16)
                {
                    msleep(10);
                    socketWaitForReadyRead(0);
                    CHECK_SOCKET_ERROR
                    if (timer.hasExpired(timeout))
                    {
                        log_error_m << "Signature of serialization type for protocol"
                                    << " is not received within " << timeout << " ms";
                        loopBreak = true;
                        break;
                    }
                }
                if (loopBreak)
                    break;

                QByteArray ba;
                ba.resize(16);
                if (socketRead((char*)ba.constData(), 16) != 16)
                {
                    log_error_m << "Failed read serialization signature";
                    loopBreak = true;
                    break;
                }

                QUuidEx incomingSignature;
                QDataStream stream {&ba, QIODevice::ReadOnly | QIODevice::Unbuffered};
                STREAM_INIT(stream);
                stream >> incomingSignature;

                if (isListenerSide())
                {
                    bool found = false;
                    for (const auto& sign : _protocolMap)
                        if (sign.second == incomingSignature)
                        {
                            found = true;
                            _messageFormat = sign.first;
                            break;
                        }

                    if (found)
                    {
                        alog::Line logLine =
                            log_verbose_m << "Message serialization format: ";
                        switch (_messageFormat)
                        {
#ifdef BPROTO_SERIALIZATION
                            case SerializationFormat::BProto:
                                logLine << "bproto";
                                break;
#endif
#ifdef JSON_SERIALIZATION
                            case SerializationFormat::Json:
                                logLine << "json";
                                break;
#endif
                            default:
                                logLine << "unknown";
                        }
                    }
                    else
                        incomingSignature = QUuidEx{};

                    // Отправка сигнатуры протокола
                    QByteArray ba;
                    QDataStream stream {&ba, QIODevice::WriteOnly};
                    STREAM_INIT(stream);
                    stream << incomingSignature;
                    socketWrite(ba.constData(), 16);
                    CHECK_SOCKET_ERROR
                    socketWaitForBytesWritten(delay);
                    CHECK_SOCKET_ERROR
                    _serializationSignatureWrite = true;

                    if (!found)
                    {
                        log_error_m << "Incompatible serialization signatures";
                        msleep(200);
                        loopBreak = true;
                        break;
                    }
                }
                else // !isListenerSide()
                {
                    if (serializationSignature != incomingSignature)
                    {
                        log_error_m << "Incompatible serialization signatures";
                        loopBreak = true;
                        break;
                    }
                }
                _serializationSignatureRead = true;
            }

            quint64 sleepCount = 0;
            socketWaitForReadyRead(0);
            CHECK_SOCKET_ERROR
            while (messagesCount() == 0
                   && readBuffSize == 0
                   && acceptMessages.empty()
                   && internalMessages.empty()
                   && socketBytesAvailable() == 0)
            {
                if (threadStop())
                {
                    loopBreak = true;
                    break;
                }
                if (socketBytesToWrite())
                {
                    socketWaitForBytesWritten(5);
                    CHECK_SOCKET_ERROR
                }

                ++sleepCount;
                int condDelay = 1;

                // Меньшее значение интервала ожидания дает лучшие  результаты
                // при синхронной  передаче большого количества маленьких сооб-
                // щений,  но при этом  может  существенно  возрасти  нагрузка
                // на процессор в режиме ожидания
                if      (sleepCount > 400) condDelay = 10; // После 1000 ms
                else if (sleepCount > 300) condDelay = 5;  // После 500 ms
                else if (sleepCount > 200) condDelay = 3;  // После 200 ms

                { //Block for QMutexLocker
                    QMutexLocker locker(&_messagesLock); (void) locker;
                    _messagesCond.wait(&_messagesLock, condDelay);
                }
                socketWaitForReadyRead(0);
                CHECK_SOCKET_ERROR
            }
            if (loopBreak)
                break;

            if (socketBytesToWrite())
            {
                socketWaitForBytesWritten(delay);
                CHECK_SOCKET_ERROR
            }

            //--- Отправка сообщений ---
            if (socketBytesToWrite() == 0)
            {
                timer.start();
                while (true)
                {
                    Message::Ptr message;
                    if (!internalMessages.empty())
                        message.attach(internalMessages.release(0));

                    if (message.empty()
                        && messagesCount() != 0
                        && _protocolCompatible == ProtocolCompatible::Yes)
                    {
                        QMutexLocker locker(&_messagesLock); (void) locker;

                        //--- Приоритезация сообщений ---
                        if (!_messagesHigh.empty())
                            message.attach(_messagesHigh.release(0));

                        if (message.empty() && !_messagesNorm.empty())
                        {
                            if (_messagesNormCounter < 5)
                            {
                                ++_messagesNormCounter;
                                message.attach(_messagesNorm.release(0));
                            }
                            else
                            {
                                _messagesNormCounter = 0;
                                if (!_messagesLow.empty())
                                    message.attach(_messagesLow.release(0));
                                else
                                    message.attach(_messagesNorm.release(0));
                            }
                        }
                        if (message.empty() && !_messagesLow.empty())
                            message.attach(_messagesLow.release(0));
                    }
                    if (loopBreak || message.empty())
                        break;

#ifdef JSON_SERIALIZATION
                    if (_messageFormat == SerializationFormat::Json
                        && !message->contentIsEmpty()
                        && message->contentFormat() != SerializationFormat::Json)
                    {
                        log_error_m << "For json packaging, message format"
                                    << " and message content format must match"
                                    << ". Message will be discarded"
                                    << ". Command: " << CommandNameLog(message->command());
                        continue;
                    }
#endif
                    if (message->command() == command::CloseConnection
                        && message->type() == Message::Type::Command)
                    {
                        commandCloseConnectionId = message->id();
                    }

                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        log_debug2_m << "Message before sending to socket"
                                     << ". Id: " << message->id()
                                     << ". Command: " << CommandNameLog(message->command());
                    }

                    BByteArray buff;
                    switch (_messageFormat)
                    {
#ifdef BPROTO_SERIALIZATION
                        case SerializationFormat::BProto:
                            buff = message->toBProto();
                            break;
#endif
#ifdef JSON_SERIALIZATION
                        case SerializationFormat::Json:
                            buff = message->toJson();
                            if (alog::logger().level() == alog::Level::Debug2)
                            {
                                log_debug2_m << "Message json before sending: "
                                             << (QByteArray)buff;
                            }
                            break;
#endif
                        default:
                            log_error_m << "Unsupported message serialization format";
                            prog_abort();
                    }
                    qint32 buffSize = buff.size();

                    if (!isLocal()
                        && message->compression() == Message::Compression::None
                        && buffSize > _compressionSize
                        && _compressionLevel != 0)
                    {
                        qint32 buffSizePrev = buffSize;
                        buff = qCompress(buff, _compressionLevel);
                        buffSize = buff.size();

                        if (alog::logger().level() == alog::Level::Debug2)
                        {
                            log_debug2_m << "Message was compressed"
                                         << ". Command: " << CommandNameLog(message->command())
                                         << " (prev size: " << buffSizePrev
                                         << "; new size: " << buffSize << ")";
                        }
                        // Передаем признак сжатия потока через знаковый бит
                        // параметра buffSize
                        buffSize *= -1;
                    }

                    if ((QSysInfo::ByteOrder != QSysInfo::BigEndian))
                        buffSize = qbswap(buffSize);

                    socketWrite((const char*)&buffSize, sizeof(qint32));
                    CHECK_SOCKET_ERROR

                    socketWrite(buff.constData(), buff.size());
                    CHECK_SOCKET_ERROR

                    while (socketBytesToWrite())
                    {
                        socketWaitForBytesWritten(5);
                        CHECK_SOCKET_ERROR
                        if (timer.hasExpired(3 * delay))
                            break;
                    }
                    if (alog::logger().level() == alog::Level::Debug2
                        && socketBytesToWrite() == 0)
                    {
                        log_debug2_m << "Message was sent to socket"
                                     << ". Id: " << message->id()
                                     << ". Command: " << CommandNameLog(message->command());
                    }
                    if (loopBreak
                        || timer.hasExpired(3 * delay))
                        break;
                }
                if (loopBreak)
                    break;
            }

            //--- Прием сообщений ---
            socketWaitForReadyRead(0);
            CHECK_SOCKET_ERROR
            timer.start();
            while (socketBytesAvailable() || readBuffSize)
            {
                if (readBuffSize == 0)
                {
                    while (socketBytesAvailable() < qint64(sizeof(qint32)))
                    {
                        socketWaitForReadyRead(1);
                        CHECK_SOCKET_ERROR
                        if (timer.hasExpired(3 * delay))
                            break;
                    }
                    if (loopBreak
                        || timer.hasExpired(3 * delay))
                        break;

                    socketRead((char*)&readBuffSize, sizeof(qint32));
                    CHECK_SOCKET_ERROR

                    if ((QSysInfo::ByteOrder != QSysInfo::BigEndian))
                        readBuffSize = qbswap(readBuffSize);

                    readBuff.resize(qAbs(readBuffSize));
                    readBuffCur = (char*)readBuff.constData();
                    readBuffEnd = readBuffCur + readBuff.size();
                }

                while (readBuffCur < readBuffEnd)
                {
                    qint64 bytesAvailable = socketBytesAvailable();
                    if (bytesAvailable == 0)
                    {
                        socketWaitForReadyRead(5);
                        CHECK_SOCKET_ERROR
                        bytesAvailable = socketBytesAvailable();
                    }
                    qint64 readBytes = qMin(bytesAvailable,
                                            qint64(readBuffEnd - readBuffCur));
                    if (readBytes != 0)
                    {
                        if (socketRead(readBuffCur, readBytes) != readBytes)
                        {
                            log_error_m << "Socket error: failed read data from socket";
                            loopBreak = true;
                            break;
                        }
                        readBuffCur += readBytes;
                    }
                    if (timer.hasExpired(3 * delay))
                        break;
                }
                if (loopBreak
                    || timer.hasExpired(3 * delay))
                    break;

                if (readBuffSize < 0)
                    readBuff = qUncompress(readBuff);

                // Обнуляем размер буфера для того, чтобы можно было начать
                // считывать новое сообщение.
                readBuffSize = 0;

                if (!readBuff.isEmpty())
                {
                    Message::Ptr message;
                    switch (_messageFormat)
                    {
#ifdef BPROTO_SERIALIZATION
                        case SerializationFormat::BProto:
                            message = Message::fromBProto(readBuff);
                            break;
#endif
#ifdef JSON_SERIALIZATION
                        case SerializationFormat::Json:
                            if (alog::logger().level() == alog::Level::Debug2)
                            {
                                log_debug2_m << "Message json received: "
                                             << (QByteArray)readBuff;
                            }
                            message = Message::fromJson(readBuff);
                            break;
#endif
                        default:
                            log_error_m << "Unsupported message deserialization format";
                            prog_abort();
                    }
                    messageInit(message);
                    readBuff.clear();

                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        log_debug2_m << "Message received"
                                     << ". Id: " << message->id()
                                     << ". Command: " << CommandNameLog(message->command());
                    }
                    if (_protocolCompatible == ProtocolCompatible::Undefined
                        && message->command() == command::ProtocolCompatible)
                    {
                        processingProtocolCompatibleCommand(message);
                        break;
                    }
                    else if (message->command() == command::CloseConnection)
                    {
                        processingCloseConnectionCommand(message);

                        // Оправляем команду во внешние обработчики
                        if (message->type() == Message::Type::Command)
                            emitMessage(message);

                        // Уходим на новый цикл, что бы как можно быстрее
                        // получить ответ по команде command::CloseConnection.
                        break;
                    }
                    else
                    {
                        if (_protocolCompatible == ProtocolCompatible::Yes)
                        {
                            acceptMessages.add(message.detach());
                        }
                        else
                        {
                            const char* proto;
                            if (_messageFormat == SerializationFormat::BProto)
                                proto = "binary";
#ifdef JSON_SERIALIZATION
                            else if (_messageFormat == SerializationFormat::Json)
                                proto = "json";
#endif
                            else
                                proto = "unknown";

                            log_error_m
                                << "Check of compatibility for " << proto << " protocol not performed"
                                << ". Command " << CommandNameLog(message->command()) << " discarded";
                        }
                    }
                }
                if (loopBreak
                    || timer.hasExpired(3 * delay))
                    break;

                socketWaitForReadyRead(0);
                CHECK_SOCKET_ERROR
            } // while (socketBytesAvailable() || readBuffSize)
            if (loopBreak)
                break;

            //--- Обработка принятых сообщений ---
            if (_protocolCompatible == ProtocolCompatible::Yes)
            {
                timer.start();
                while (!acceptMessages.empty())
                {
                    Message::Ptr m;
                    m.attach(acceptMessages.release(0));

                    if (_checkUnknownCommands)
                    {
                        // Обработка уведомления о неизвестной команде
                        if (m->command() == command::Unknown)
                        {
                            data::Unknown unknown;
                            readFromMessage(m, unknown);
                            if (unknown.dataIsValid)
                            {
                                alog::Line logLine = log_error_m
                                    << "Command " << CommandNameLog(unknown.commandId)
                                    << " is unknown for remote side"
                                    << ". Socket descriptor: " << unknown.socketDescriptor;
                                if (unknown.socketType == SocketType::Tcp)
                                    logLine << ". Host: " << unknown.address << ":" << unknown.port;
                                else if (unknown.socketType == SocketType::Local)
                                    logLine << ". Socket name: " << unknown.socketName;
                                else
                                    logLine << ". Unsupported socket type";

                                SpinLocker locker(_unknownCommandsLock); (void) locker;
                                _unknownCommands.insert(unknown.commandId);
                            }
                            else
                                log_error_m << "Incorrect data structure for command "
                                            << CommandNameLog(m->command());
                            continue;
                        }

                        // Если команда неизвестна - отправляем об этом уведомление
                        // и переходим к обработке следующей команды.
                        if (!command::pool().commandExists(m->command()))
                        {
                            data::Unknown unknown;
                            fillUnknownMessage(m, unknown);
                            Message::Ptr mUnknown = createMessage(unknown, {_messageFormat});
                            mUnknown->setPriority(Message::Priority::High);
                            internalMessages.add(mUnknown.detach());

                            alog::Line logLine = log_error_m
                                << "Unknown command: " << unknown.commandId
                                << ". Socket descriptor: " << unknown.socketDescriptor;
                            if (unknown.socketType == SocketType::Tcp)
                                logLine << ". Host: " << unknown.address << ":" << unknown.port;
                            else if (unknown.socketType == SocketType::Local)
                                logLine << ". Socket name: " << unknown.socketName;
                            else
                                logLine << ". Unsupported socket type";

                            continue;
                        }
                    }

                    emitMessage(m);
                    if (timer.hasExpired(3 * delay))
                        break;
                }
            }
        } // while (true)
    }
    catch (std::exception& e)
    {
        log_error_m << "Detail: " << e.what();
    }
    catch (...)
    {
        log_error_m << "Unknown error";
    }

    { // Block for SpinLocker
        SpinLocker locker(_socketLock); (void) locker;
        socketClose();
    }
    _initSocketDescriptor = {-1};

    #undef CHECK_SOCKET_ERROR
}

void Socket::emitMessage(const communication::Message::Ptr& m)
{
    try
    {
        if (alog::logger().level() == alog::Level::Debug2)
            log_debug2_m << "Message emit"
                         << ". Id: " << m->id()
                         << ". Command: " << CommandNameLog(m->command());
        emit message(m);
    }
    catch (std::exception& e)
    {
        log_error_m << "Failed processing message. Detail: " << e.what();
    }
    catch (...)
    {
        log_error_m << "Failed processing message. Unknown error";
    }
}

//-------------------------------- Listener ----------------------------------

Socket::List Listener::sockets() const
{
    Socket::List sockets;
    QMutexLocker locker(&_socketsLock); (void) locker;
    for (Socket* s : _sockets)
        if (s->isRunning())
        {
            s->add_ref();
            sockets.add(s);
        }

    return std::move(sockets);
}

void Listener::send(const Message::Ptr& message,
                    const SocketDescriptorSet& excludeSockets) const
{
    Socket::List sockets = this->sockets();
    communication::transport::send(sockets, message, excludeSockets);
}

void Listener::send(const Message::Ptr& message,
                    SocketDescriptor excludeSocket) const
{
    Socket::List sockets = this->sockets();
    SocketDescriptorSet excludeSockets;
    excludeSockets << excludeSocket;
    communication::transport::send(sockets, message, excludeSockets);
}

Socket::Ptr Listener::socketByDescriptor(SocketDescriptor descr) const
{
    QMutexLocker locker(&_socketsLock); (void) locker;
    for (Socket* s : _sockets)
        if (s->socketDescriptor() == descr)
            return Socket::Ptr(s);

    return Socket::Ptr();
}

void Listener::addSocket(const Socket::Ptr& socket)
{
    if (socket.empty()
        || socket->socketDescriptor() == SocketDescriptor(-1))
        return;

    QMutexLocker locker(&_socketsLock); (void) locker;
    bool socketExists = false;
    for (int i = 0; i < _sockets.count(); ++i)
        if (_sockets.item(i)->socketDescriptor() == socket->socketDescriptor())
        {
            socketExists = true;
            break;
        }

    if (!socketExists)
    {
        socket->add_ref();
        _sockets.add(socket);
        connectSignals(socket.get());
    }
}

Socket::Ptr Listener::releaseSocket(SocketDescriptor descr)
{
    QMutexLocker locker(&_socketsLock); (void) locker;
    Socket::Ptr socket;
    for (int i = 0; i < _sockets.count(); ++i)
        if (_sockets.item(i)->socketDescriptor() == descr)
        {
            socket.attach(_sockets.release(i));
            disconnectSignals(socket.get());
            break;
        }

    return std::move(socket);
}

void Listener::closeSockets()
{
    _removeClosedSockets.stop();
    Socket::List sockets = this->sockets();
    for (Socket* s : sockets)
        if (s->isRunning())
            s->stop();
}

void Listener::removeClosedSocketsInternal()
{
    QMutexLocker locker(&_socketsLock); (void) locker;
    for (int i = 0; i < _sockets.count(); ++i)
        if (!_sockets.item(i)->isRunning())
            _sockets.remove(i--);
}

void Listener::incomingConnectionInternal(Socket::Ptr socket,
                                          SocketDescriptor socketDescriptor)
{
    socket->setListenerSide(true);
    socket->setInitSocketDescriptor(socketDescriptor);
    socket->setCompressionLevel(_compressionLevel);
    socket->setCompressionSize(_compressionSize);
    socket->setCheckProtocolCompatibility(_checkProtocolCompatibility);
    socket->setCheckUnknownCommands(_checkUnknownCommands);

    connectSignals(socket.get());

    // Примечание: выход из функции start() происходит только тогда, когда
    // поток гарантированно запустится, поэтому случайное удаление нового
    // сокета в функции removeClosedSockets() исключено.
    socket->start();

    QMutexLocker locker(&_socketsLock); (void) locker;
    socket->add_ref();
    _sockets.add(socket.get());
    socket->setInsideListener(true);
}

} // namespace base

void send(const base::Socket::List& sockets,
          const Message::Ptr& message,
          const SocketDescriptorSet& excludeSockets)
{
    if (message->type() == Message::Type::Event)
    {
        for (base::Socket* s : sockets)
            if (!excludeSockets.contains(s->socketDescriptor()))
                s->send(message);
    }
    else
    {
        if (!message->destinationSocketDescriptors().isEmpty())
        {
            bool messageSended = false;
            for (base::Socket* s : sockets)
                if (message->destinationSocketDescriptors().contains(s->socketDescriptor()))
                {
                    s->send(message);
                    messageSended = true;
                }
            if (!messageSended)
            {
                alog::Line logLine =
                    log_error_m << "Impossible send message: " << CommandNameLog(message->command())
                                << ". Not found sockets with descriptors:";
                for (SocketDescriptor sd : message->destinationSocketDescriptors())
                    logLine << " " << sd;
                logLine << ". Message will be discarded";
            }
        }
        else if (message->socketDescriptor() != SocketDescriptor(-1))
        {
            bool messageSended = false;
            for (base::Socket* s : sockets)
                if (s->socketDescriptor() == message->socketDescriptor()
                    && s->type() == message->socketType())
                {
                    s->send(message);
                    messageSended = true;
                    break;
                }
            if (!messageSended)
                log_error_m << "Impossible send message: " << CommandNameLog(message->command())
                            << ". Not found socket with descriptor: " << message->socketDescriptor()
                            << ". Message will be discarded";
        }
        else
        {
            log_error_m << "Impossible send message: " << CommandNameLog(message->command())
                        << ". Destination socket descriptors is undefined"
                        << ". Message will be discarded";
        }
    }
}

void send(const base::Socket::List& sockets,
          const Message::Ptr& message,
          SocketDescriptor excludeSocket)
{
    SocketDescriptorSet excludeSockets;
    excludeSockets << excludeSocket;
    send(sockets, message, excludeSockets);
}

base::Socket::List concatSockets(const base::Listener& listener)
{
    return listener.sockets();
}

} // namespace transport
} // namespace communication

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
