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

//-------------------------------- Socket ------------------------------------

const QUuidEx Socket::_protocolSignature = {"82c40273-4037-4f1b-a823-38123435b22f"};

Socket::Socket(SocketType type) : _type(type)
{
    registrationQtMetatypes();
}

bool Socket::isConnected() const
{
    return (socketIsConnected()
            && _binaryProtocolStatus == BinaryProtocol::Compatible);
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

Socket::BinaryProtocol Socket::binaryProtocolStatus() const
{
    return _binaryProtocolStatus;
}

SocketDescriptor Socket::socketDescriptor() const
{
    SpinLocker locker(_socketLock); (void) locker;
    return socketDescriptorInternal();
}

bool Socket::send(const Message::Ptr& message)
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
        log_error_m << "Cannot send a empty message";
        return false;
    }
    if (_checkUnknownCommands)
    {
        bool isUnknown;
        { //Block for SpinLocker
            SpinLocker locker(_unknownCommandsLock); (void) locker;
            isUnknown = (_unknownCommands.constFind(message->command()) != _unknownCommands.constEnd());
        }
        if (isUnknown)
        {
            log_error_m << "Command " << CommandNameLog(message->command())
                        << " is unknown for the receiving side"
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
        ++_messagesCount;
    }
    if (alog::logger().level() == alog::Level::Debug2)
    {
        log_debug2_m << "Message added to a queue to sending"
                     << "; message id: " << message->id()
                     << "; command: " << CommandNameLog(message->command());
    }
    return true;
}

bool Socket::send(const QUuidEx& command)
{
    Message::Ptr message = Message::create(command);
    return send(message);
}

void Socket::remove(const QUuidEx& command)
{
    QMutexLocker locker(&_messagesLock); (void) locker;
    auto remove_cond = [&command](Message* m) -> bool
    {
        bool res = (command == m->command());
        if (res && (alog::logger().level() == alog::Level::Debug2))
            log_debug2_m << "Message removed from a queue to sending"
                         << "; message id: " << m->id()
                         << "; command: " << CommandNameLog(m->command());
        return res;
    };
    _messagesHigh.removeCond(remove_cond);
    _messagesNorm.removeCond(remove_cond);
    _messagesLow.removeCond(remove_cond);

    _messagesCount = _messagesHigh.count()
                     + _messagesNorm.count()
                     + _messagesLow.count();
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
    _protocolSignatureRead = false;
    _protocolSignatureWrite = false;

    emit disconnected(_initSocketDescriptor);
    _initSocketDescriptor = -1;
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

void Socket::run()
{
    { // Block for SpinLocker
        SpinLocker locker(_socketLock); (void) locker;
        socketCreate();
    }

    if (!socketInit())
    {
        _initSocketDescriptor = -1;
        return;
    }
    _initSocketDescriptor = socketDescriptorInternal();

    Message::List internalMessages;
    Message::List acceptMessages;

    QElapsedTimer timer;
    bool loopBreak = false;
    qint32 readBuffSize = 0;
    const int delay = 50;

    // Идентификатор команды CloseConnection, используется для отслеживания
    // ответа на запрос о разрыве соединения.
    QUuidEx commandCloseConnectionId;

    _binaryProtocolStatus = BinaryProtocol::Undefined;

    // Добавляем самое первое сообщение с информацией о совместимости
    {
        //data::ProtocolCompatible protocolCompatible;
        //protocolCompatible.versionLow = BPROTOCOL_VERSION_LOW;
        //protocolCompatible.versionHigh = BPROTOCOL_VERSION_HIGH;
        //Message::Ptr message = createMessage(protocolCompatible);
        Message::Ptr message = Message::create(command::ProtocolCompatible);
        message->setPriority(Message::Priority::High);
        internalMessages.add(message.detach());
    }

    auto processingProtocolCompatibleCommand = [&](Message::Ptr& message) -> void
    {
        if (message->type() == Message::Type::Command)
        {
            quint16 protocolVersionLow  = message->protocolVersionLow();
            quint16 protocolVersionHigh = message->protocolVersionHigh();

            _binaryProtocolStatus = BinaryProtocol::Compatible;
            if (_checkProtocolCompatibility)
            {
                if (alog::logger().level() >= alog::Level::Debug)
                {
                    log_debug_m
                        << "Checking binary protocol compatibility"
                        << ". This protocol version: "
                        << BPROTOCOL_VERSION_LOW << "-" << BPROTOCOL_VERSION_HIGH
                        << ". Remote protocol version: "
                        << protocolVersionLow << "-" << protocolVersionHigh;
                }
                if (!communication::protocolCompatible(protocolVersionLow,
                                                       protocolVersionHigh))
                    _binaryProtocolStatus = BinaryProtocol::Incompatible;
            }

//            data::ProtocolCompatible pcompatible;
//            readFromMessage(message, pcompatible);
//            if (pcompatible.isValid)
//            {
//                if (alog::logger().level() >= alog::Level::Debug)
//                {
//                    log_debug_m
//                        << "Checking binary protocol compatibility"
//                        << ". This protocol version: "
//                        << BPROTOCOL_VERSION_LOW << "-" << BPROTOCOL_VERSION_HIGH
//                        << "; Remote protocol version: "
//                        << pcompatible.versionLow << "-" << pcompatible.versionHigh;
//                }
//                if (communication::protocolCompatible(pcompatible.versionLow,
//                                                      pcompatible.versionHigh))
//                    _binaryProtocolStatus = BinaryProtocol::Compatible;
//            }
//            else
//            {
//                log_error_m << "Incorrect data structure for command "
//                            << CommandNameLog(message->command());
//            }

            if (_binaryProtocolStatus == BinaryProtocol::Compatible)
            {
                emit connected(socketDescriptorInternal());
            }
            else // BinaryProtocol::Incompatible
            {
                data::CloseConnection closeConnection;
                closeConnection.description = QString(
                    "Binary protocol versions incompatible"
                    ". This protocol version: %1-%2"
                    ". Remote protocol version: %3-%4"
                )
                .arg(BPROTOCOL_VERSION_LOW).arg(BPROTOCOL_VERSION_HIGH)
                .arg(protocolVersionLow).arg(protocolVersionHigh);

                log_verbose_m << "Send request to close the connection"
                              << ". Detail: " << closeConnection.description;

                Message::Ptr m = createMessage(closeConnection);
                commandCloseConnectionId = m->id();
                m->setPriority(Message::Priority::High);
                internalMessages.add(m.detach());
            }
        }
    };

    auto processingCloseConnectionCommand = [&](Message::Ptr& message) -> void
    {
        if (message->type() == Message::Type::Command)
        {
            data::CloseConnection closeConnection;
            readFromMessage(message, closeConnection);
            if (closeConnection.isValid)
                log_verbose_m << "Connection will be closed at the request remote side"
                              << ". Remote detail: " << closeConnection.description;
            else
                log_error_m << "Incorrect data structure for command "
                            << CommandNameLog(message->command());

            // Отправляем ответ
            message->clearContent();
            message->setType(Message::Type::Answer);
            message->setExecStatus(Message::ExecStatus::Success);
            message->setPriority(Message::Priority::High);
            message->add_ref();
            internalMessages.add(message.get());
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
            if (!_protocolSignatureWrite)
            {
                QByteArray ba;
                QDataStream s {&ba, QIODevice::WriteOnly};
                s << _protocolSignature;
                socketWrite(ba.constData(), 16);
                CHECK_SOCKET_ERROR
                socketWaitForBytesWritten(delay);
                CHECK_SOCKET_ERROR
                _protocolSignatureWrite = true;
            }

            // Проверка сигнатуры протокола
            if (!_protocolSignatureRead)
            {
                timer.start();
                while (socketBytesAvailable() < 16)
                {
                    socketWaitForReadyRead(delay);
                    CHECK_SOCKET_ERROR
                    static const int timeout {40 * delay};
                    if (timer.hasExpired(timeout))
                    {
                        log_error_m << "The signature of protocol is not "
                                    << "received within " << timeout << " ms";
                        loopBreak = true;
                        break;
                    }
                }
                if (loopBreak)
                    break;

                QByteArray ba;
                ba.resize(16);
                if (socketRead((char*)ba.data(), 16) != 16)
                {
                    log_error_m << "Failed read protocol signature";
                    loopBreak = true;
                    break;
                }

                QUuidEx protocolSignature;
                QDataStream s {&ba, QIODevice::ReadOnly | QIODevice::Unbuffered};
                s >> protocolSignature;
                if (_protocolSignature != protocolSignature)
                {
                    log_error_m << "Incompatible protocol signatures";
                    loopBreak = true;
                    break;
                }
                _protocolSignatureRead = true;
            }

            { //Block for QMutexLocker
                QMutexLocker locker(&_messagesLock); (void) locker;
                _messagesCount = _messagesHigh.count()
                                 + _messagesNorm.count()
                                 + _messagesLow.count();
            }

            while (socketBytesAvailable() == 0
                   && _messagesCount == 0
                   && internalMessages.empty()
                   && acceptMessages.empty())
            {
                if (threadStop())
                {
                    loopBreak = true;
                    break;
                }
                if (socketBytesToWrite())
                    socketWaitForBytesWritten(20);
                socketWaitForReadyRead(20);
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
                        && _messagesCount != 0
                        && _binaryProtocolStatus == BinaryProtocol::Compatible)
                    {
                        QMutexLocker locker(&_messagesLock); (void) locker;
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

                        _messagesCount = _messagesHigh.count()
                                         + _messagesNorm.count()
                                         + _messagesLow.count();
                    }
                    if (loopBreak || message.empty())
                        break;

                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        log_debug2_m << "Message before sending to the socket"
                                     << "; message id: " << message->id()
                                     << "; command: " << CommandNameLog(message->command());
                    }
                    QByteArray buff = message->toByteArray();
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
                                         << "; command: " << CommandNameLog(message->command())
                                         << "; prev size: " << buffSizePrev
                                         << "; new size: " << buffSize;
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
                        socketWaitForBytesWritten(delay);
                        CHECK_SOCKET_ERROR
                        if (timer.hasExpired(3 * delay))
                            break;
                    }
                    if (alog::logger().level() == alog::Level::Debug2
                        && socketBytesToWrite() == 0)
                    {
                        log_debug2_m << "Message was send to the socket"
                                     << "; message id: " << message->id()
                                     << "; command: " << CommandNameLog(message->command());
                    }
                    if (loopBreak
                        || timer.hasExpired(3 * delay))
                        break;
                }
                if (loopBreak)
                    break;
            }

            //--- Прием сообщений ---
            if (socketBytesAvailable() != 0)
            {
                timer.start();
                while (true)
                {
                    if (qAbs(readBuffSize) == 0)
                    {
                        while (socketBytesAvailable() < (int)sizeof(qint32))
                        {
                            socketWaitForReadyRead(delay);
                            CHECK_SOCKET_ERROR
                            if (timer.hasExpired(3 * delay))
                                break;
                        }
                        if (loopBreak
                            || timer.hasExpired(3 * delay)
                            || socketBytesAvailable() < (int)sizeof(qint32))
                            break;

                        socketRead((char*)&readBuffSize, sizeof(qint32));
                        CHECK_SOCKET_ERROR

                        if ((QSysInfo::ByteOrder != QSysInfo::BigEndian))
                            readBuffSize = qbswap(readBuffSize);
                    }

                    while (socketBytesAvailable() < qAbs(readBuffSize))
                    {
                        socketWaitForReadyRead(delay);
                        CHECK_SOCKET_ERROR
                        if (timer.hasExpired(3 * delay))
                            break;
                    }
                    if (loopBreak
                        || timer.hasExpired(3 * delay)
                        || socketBytesAvailable() < qAbs(readBuffSize))
                        break;

                    BByteArray buff;
                    buff.resize(qAbs(readBuffSize));
                    if (socketRead((char*)buff.data(), qAbs(readBuffSize)) != qAbs(readBuffSize))
                    {
                        log_error_m << "Socket error: failed read data from socket";
                        loopBreak = true;
                        break;
                    }
                    if (readBuffSize < 0)
                        buff = qUncompress(buff);

                    // Обнуляем размер буфера для того, чтобы можно было начать
                    // считывать новое сообщение.
                    readBuffSize = 0;

                    if (!buff.isEmpty())
                    {
                        Message::Ptr message = messageFromByteArray(buff);

                        if (alog::logger().level() == alog::Level::Debug2)
                        {
                            log_debug2_m << "Message received"
                                         << "; message id: " << message->id()
                                         << "; command: " << CommandNameLog(message->command());
                        }
                        if (_binaryProtocolStatus == BinaryProtocol::Undefined
                            && message->command() == command::ProtocolCompatible)
                        {
                            processingProtocolCompatibleCommand(message);
                            break;
                        }
                        else if (message->command() == command::CloseConnection)
                        {
                            processingCloseConnectionCommand(message);
                            // Уходим на новый цикл, что бы как можно быстрее
                            // получить ответ по команде command::CloseConnection.
                            break;
                        }
                        else
                        {
                            if (_binaryProtocolStatus == BinaryProtocol::Compatible)
                            {
                                acceptMessages.add(message.detach());
                            }
                            else
                            {
                                log_error_m
                                    << "Check of compatibility for the binary protocol not performed"
                                    << ". Command " << CommandNameLog(message->command()) << " discarded";
                            }
                        }
                    }
                    if (loopBreak
                        || timer.hasExpired(3 * delay))
                        break;
                } // while (true)
                if (loopBreak)
                    break;
            }

            //--- Обработка принятых сообщений ---
            if (_binaryProtocolStatus == BinaryProtocol::Compatible)
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
                            if (unknown.isValid)
                            {
                                alog::Line logLine = log_error_m
                                    << "Command " << CommandNameLog(unknown.commandId)
                                    << " is unknown for the remote side"
                                    << "; socket descriptor: " << unknown.socketDescriptor;
                                if (unknown.socketType == SocketType::Tcp)
                                    logLine << "; host: " << unknown.address << ":" << unknown.port;
                                else if (unknown.socketType == SocketType::Local)
                                    logLine << "; socket name: " << unknown.socketName;
                                else
                                    logLine << "; unsupported socket type";

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
                        if (!commandsPool().commandExists(m->command()))
                        {
                            data::Unknown unknown;
                            fillUnknownMessage(m, unknown);
                            Message::Ptr mUnknown = createMessage(unknown);
                            mUnknown->setPriority(Message::Priority::High);
                            internalMessages.add(mUnknown.detach());

                            alog::Line logLine = log_error_m
                                << "Unknown command: " << unknown.commandId
                                << "; socket descriptor: " << unknown.socketDescriptor;
                            if (unknown.socketType == SocketType::Tcp)
                                logLine << "; host: " << unknown.address << ":" << unknown.port;
                            else if (unknown.socketType == SocketType::Local)
                                logLine << "; socket name: " << unknown.socketName;
                            else
                                logLine << "; unsupported socket type";

                            continue;
                        }
                    }

                    try
                    {
                        if (alog::logger().level() == alog::Level::Debug2)
                            log_debug2_m << "Message emit"
                                         << "; message id: " << m->id()
                                         << "; command: " << CommandNameLog(m->command());
                        emit message(m);
                    }
                    catch (std::exception& e)
                    {
                        log_error_m << "Failed processing a message. Detail: " << e.what();
                    }
                    catch (...)
                    {
                        log_error_m << "Failed processing a message. Unknown error";
                    }
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
    _initSocketDescriptor = -1;

    #undef CHECK_SOCKET_ERROR
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
        {
            s->stop();
            s->wait();
        }
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
    _sockets.add(socket.detach());
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
