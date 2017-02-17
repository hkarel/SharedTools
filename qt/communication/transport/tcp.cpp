#include "break_point.h"
#include "spin_locker.h"
#include "logger/logger.h"
#include "qt/logger/logger_operators.h"
#include "qt/version/version_number.h"
#include "qt/communication/commands_base.h"
#include "qt/communication/commands_pool.h"
#include "qt/communication/transport/tcp.h"
#include "qt/communication/logger_operators.h"

#include <stdexcept>

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportTCP")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportTCP")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportTCP")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportTCP")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportTCP")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportTCP")

namespace communication {
namespace transport {
namespace tcp {

//--------------------------------- Base -------------------------------------

void Base::setCompressionLevel(int val)
{
    _compressionLevel = qBound(-1, val, 9);
}

//-------------------------------- Socket ------------------------------------

const QUuidEx Socket::_protocolSignature = {"82c40273-4037-4f1b-a823-38123435b22f"};

bool Socket::isConnected() const
{
    return (socketIsConnected()
            && (_binaryProtocolStatus == BinaryProtocol::Compatible));
}

void Socket::setSocketDescriptor(SocketDescriptor socketDescriptor)
{
    _socketDescriptor = socketDescriptor;
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
                     << ". Command " << CommandNameLog(message->command());
    }
    return true;
}

bool Socket::send(const QUuidEx& command)
{
    Message::Ptr message = Message::create(command);
    return send(message);
}

bool Socket::socketIsConnected() const
{
    return (_socket
            && _socket->isValid()
            && (_socket->state() == QAbstractSocket::ConnectedState));
}

bool Socket::isLoopback() const
{
#if QT_VERSION >= 0x050000
    return (_socket && _socket->peerAddress().isLoopback());
#else
    return (_socket && (_socket->peerAddress() == QHostAddress::LocalHost));
#endif
}

void Socket::remove(const QUuidEx& command)
{
    QMutexLocker locker(&_messagesLock); (void) locker;
    auto remove_cond = [&command](Message* m) -> bool
    {
        bool res = (command == m->command());
        if (res && (alog::logger().level() == alog::Level::Debug2))
            log_debug2_m << "Message removed from a queue to sending."
                         << " Command " << CommandNameLog(m->command());
        return res;
    };
    _messagesHigh.removeCond(remove_cond);
    _messagesNorm.removeCond(remove_cond);
    _messagesLow.removeCond(remove_cond);

    _messagesCount = _messagesHigh.count()
                     + _messagesNorm.count()
                     + _messagesLow.count();
}

Socket::BinaryProtocol Socket::binaryProtocolStatus() const
{
    return _binaryProtocolStatus;
}

SocketDescriptor Socket::socketDescriptor() const
{
    return (_socket) ? _socket->socketDescriptor() : -1;
}

void Socket::socketDisconnected()
{
    _protocolSignatureRead = false;
    _protocolSignatureWrite = false;
    emit disconnected(_socketDescriptor);
}

void Socket::run()
{
    const char* connectDirection = "to";

    if (_socket.empty())
    {
        _socket = simple_ptr<QTcpSocket>(new QTcpSocket(0));
        chk_connect_d(_socket.get(), SIGNAL(disconnected()), this, SLOT(socketDisconnected()))
    }

    if ((_socket->socketDescriptor() == -1) && (_socketDescriptor != -1))
    {
        connectDirection = "from";
        if (!_socket->setSocketDescriptor(_socketDescriptor))
        {
            log_error_m << "Failed set socket descriptor"
                        << "; Error code: " << int(_socket->error())
                        << "; Detail: " << _socket->errorString();
            return;
        }
    }
    _socketDescriptor = _socket->socketDescriptor();
    _address = _socket->peerAddress();
    _port = _socket->peerPort();

    log_verbose_m << "Connect " << connectDirection << " host "
                  << _address << ":" << _port
                  << "; Socket descriptor: " << _socketDescriptor;

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
                        << "; Remote protocol version: "
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
                emit connected(_socket->socketDescriptor());
            }
            else // BinaryProtocol::Incompatible
            {
                data::CloseConnection closeConnection;
                closeConnection.description = QString(
                    "Binary protocol versions incompatible"
                    ". This protocol version: %1-%2"
                    "; Remote protocol version: %3-%4"
                )
                .arg(BPROTOCOL_VERSION_LOW).arg(BPROTOCOL_VERSION_HIGH)
                .arg(protocolVersionLow).arg(protocolVersionHigh);

                log_verbose_m << "Send request to close the connection"
                              << "; Detail: " << closeConnection.description;

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
                              << "; Remote detail: " << closeConnection.description;
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

    auto socketIsActual = [this]() -> bool
    {
        return (this->_socket->isValid()
                && (this->_socket->state() != QAbstractSocket::UnconnectedState));
    };

    #define CHECK_SOCKET_ERROR \
        if (!socketIsActual()) \
        { \
            if (_socket->error() == QAbstractSocket::RemoteHostClosedError) { \
                log_verbose_m << _socket->errorString() \
                              << "; Remote host: " << _address << ":" << _port \
                              << "; Socket descriptor: " << _socketDescriptor; \
            } else { \
                log_error_m << "Socket error code: " << int(_socket->error()) \
                            << "; Detail: " << _socket->errorString(); \
            } \
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
                _socket->write(ba.constData(), 16);
                CHECK_SOCKET_ERROR
                _socket->waitForBytesWritten(delay);
                CHECK_SOCKET_ERROR
                _protocolSignatureWrite = true;
            }

            // Проверка сигнатуры протокола
            if (!_protocolSignatureRead)
            {
                timer.start();
                while (_socket->bytesAvailable() < 16)
                {
                    _socket->waitForReadyRead(delay);
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
                if (_socket->read((char*)ba.data(), 16) != 16)
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

            while (_socket->bytesAvailable() == 0
                   && _messagesCount == 0
                   && internalMessages.empty()
                   && acceptMessages.empty())
            {
                if (threadStop())
                {
                    loopBreak = true;
                    break;
                }
                if (_socket->bytesToWrite())
                    _socket->waitForBytesWritten(20);
                _socket->waitForReadyRead(20);
                CHECK_SOCKET_ERROR
            }
            if (loopBreak)
                break;

            if (_socket->bytesToWrite())
            {
                _socket->waitForBytesWritten(delay);
                CHECK_SOCKET_ERROR
            }

            //--- Отправка сообщений ---
            if (_socket->bytesToWrite() == 0)
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
                                     << ". Command " << CommandNameLog(message->command());
                    }
                    QByteArray buff = message->toByteArray();
                    qint32 buffSize = buff.size();

                    if (!isLoopback()
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
                                         << ". Command " << CommandNameLog(message->command())
                                         << " Prev size: " << buffSizePrev
                                         << " New size: " << buffSize;
                        }
                        // Передаем признак сжатия потока через знаковый бит
                        // параметра buffSize
                        buffSize *= -1;
                    }

                    if ((QSysInfo::ByteOrder != QSysInfo::BigEndian))
                        buffSize = qbswap(buffSize);

                    _socket->write((const char*)&buffSize, sizeof(qint32));
                    CHECK_SOCKET_ERROR

                    _socket->write(buff.constData(), buff.size());
                    CHECK_SOCKET_ERROR

                    while (_socket->bytesToWrite())
                    {
                        _socket->waitForBytesWritten(delay);
                        CHECK_SOCKET_ERROR
                        if (timer.hasExpired(3 * delay))
                            break;
                    }
                    if (alog::logger().level() == alog::Level::Debug2
                        && _socket->bytesToWrite() == 0)
                        //&& timer.hasExpired(3 * delay))
                    {
                        log_debug2_m << "Message was send to the socket"
                                     << ". Command " << CommandNameLog(message->command());
                    }
                    if (loopBreak
                        || timer.hasExpired(3 * delay))
                        break;
                }
                if (loopBreak)
                    break;
            }

            //--- Прием сообщений ---
            if (_socket->bytesAvailable() != 0)
            {
                timer.start();
                while (true)
                {
                    if (qAbs(readBuffSize) == 0)
                    {
                        while (_socket->bytesAvailable() < (int)sizeof(qint32))
                        {
                            _socket->waitForReadyRead(delay);
                            CHECK_SOCKET_ERROR
                            if (timer.hasExpired(3 * delay))
                                break;
                        }
                        if (loopBreak
                            || timer.hasExpired(3 * delay)
                            || _socket->bytesAvailable() < (int)sizeof(qint32))
                            break;

                        _socket->read((char*)&readBuffSize, sizeof(qint32));
                        CHECK_SOCKET_ERROR

                        if ((QSysInfo::ByteOrder != QSysInfo::BigEndian))
                            readBuffSize = qbswap(readBuffSize);
                    }

                    while (_socket->bytesAvailable() < qAbs(readBuffSize))
                    {
                        _socket->waitForReadyRead(delay);
                        CHECK_SOCKET_ERROR
                        if (timer.hasExpired(3 * delay))
                            break;
                    }
                    if (loopBreak
                        || timer.hasExpired(3 * delay)
                        || _socket->bytesAvailable() < qAbs(readBuffSize))
                        break;

                    BByteArray buff;
                    buff.resize(qAbs(readBuffSize));
                    if (_socket->read((char*)buff.data(), qAbs(readBuffSize)) != qAbs(readBuffSize))
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
                        Message::Ptr message = Message::fromByteArray(buff);
                        message->setSocketDescriptor(_socket->socketDescriptor());
                        message->setSourcePoint({_socket->peerAddress(), _socket->peerPort()});

                        if (alog::logger().level() == alog::Level::Debug2)
                        {
                            log_debug2_m << "Message received"
                                         << ". Command " << CommandNameLog(message->command());
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
                                //message->add_ref();
                                //acceptMessages.add(message.get());
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
                                log_error_m << "Command " << CommandNameLog(unknown.commandId)
                                            << " is unknown for the remote side"
                                            << "; Remote host:" << unknown.address << ":" << unknown.port
                                            << "; Socket descriptor: " << unknown.socketDescriptor;

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
                            unknown.commandId = m->command();
                            unknown.address = _socket->peerAddress();
                            unknown.port = _socket->peerPort();
                            unknown.socketDescriptor = _socket->socketDescriptor();
                            Message::Ptr mUnknown = createMessage(unknown);
                            mUnknown->setPriority(Message::Priority::High);
                            internalMessages.add(mUnknown.detach());
                            log_error_m << "Unknown command: " << unknown.commandId
                                        << "; Host: " << unknown.address << ":" << unknown.port
                                        << "; Socket descriptor: " << unknown.socketDescriptor;
                            continue;
                        }
                    }

                    try
                    {
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

    try
    {
        if (socketIsActual())
        {
            log_verbose_m << "Disconnected from host "
                          << _socket->peerAddress() << ":" << _socket->peerPort()
                          << "; Socket descriptor: " << _socket->socketDescriptor();

            _socket->disconnectFromHost();
            if (_socket->state() != QAbstractSocket::UnconnectedState)
                _socket->waitForDisconnected(1000);
        }
        _socket->close();
    }
    catch (std::exception& e)
    {
        log_error_m << "Detail: " << e.what();
    }
    catch (...)
    {
        log_error_m << "Unknown error";
    }

    #undef CHECK_SOCKET_ERROR
}

//-------------------------------- Sender ------------------------------------

Sender::Sender()
{
    registrationQtMetatypes();
}

Sender::~Sender()
{
}

bool Sender::init(const QHostAddress& address, int port)
{
    if (isRunning())
    {
        log_error_m << "Impossible execute a initialization because Sender thread is running.";
        return false;
    }
    if (port < 1 || port > 65535)
    {
        log_error_m << "A port must be in interval 1 - 65535. Assigned value: " << port;
        return false;
    }
    _address = address;
    _port = quint16(port);
    return true;
}

void Sender::connect()
{
    start();
}

void Sender::disconnect()
{
    stop();
}

void Sender::run()
{
    _socket = simple_ptr<QTcpSocket>(new QTcpSocket(0));
    chk_connect_d(_socket.get(), SIGNAL(disconnected()), this, SLOT(socketDisconnected()))

    _socketDescriptor = -1;

    while (true)
    {
        if (threadStop())
            break;

        log_verbose_m << "Try connect: " << _address << ":" << _port;

        _socket->connectToHost(_address, _port);
        if (!_socket->waitForConnected(_waitConnection * 1000))
        {
            log_error_m << "Failed connect to host "
                        << _address << ":" << _port
                        << "; Error code: " << int(_socket->error())
                        << "; Detail: " << _socket->errorString();
            //sleep(10);
            continue;
        }
        Socket::run();

        if (_socket->socketDescriptor() == -1)
            break;
        if (binaryProtocolStatus() == BinaryProtocol::Incompatible)
            break;
    }
}

//------------------------------- Listener -----------------------------------

Listener::Listener() : QTcpServer(0)
{
    registrationQtMetatypes();
    chk_connect_q(&_removeClosedSockets, SIGNAL(timeout()),
                  this, SLOT(removeClosedSockets()))
}

bool Listener::init(const QHostAddress& address, int port)
{
    if (port < 1 || port > 65535)
    {
        log_error_m << "A port must be in interval 1 - 65535. Assigned value: " << port;
        return false;
    }

    int attempt = 0;
    while (!QTcpServer::listen(address, quint16(port)))
    {
        if (++attempt > 10)
            break;
        QThread::usleep(100);
    }
    if (attempt > 10)
        log_error_m << "Start listener of connection is failed";
    else
        log_verbose_m << "Start listener of connection with params: "
                      << serverAddress() << ":" << serverPort();

    _removeClosedSockets.start(15*1000);
    return (attempt <= 10);
}

void Listener::close()
{
    _removeClosedSockets.stop();
    QVector<Socket::Ptr> sockets = this->sockets();
    for (Socket::Ptr s : sockets)
        if (s->isRunning())
        {
            s->stop();
            s->wait();
        }
    QTcpServer::close();
    log_verbose_m << "Stop listener of connection";
}

QVector<Socket::Ptr> Listener::sockets() const
{
    QVector<Socket::Ptr> sockets;
    QMutexLocker locker(&_socketsLock); (void) locker;
    for (const Socket::Ptr& s : _sockets)
        if (s->isRunning())
            sockets.append(s);
    return sockets;
}

void Listener::send(const Message::Ptr& message,
                    const SocketDescriptorSet& excludeSockets) const
{
    QVector<Socket::Ptr> sockets = this->sockets();
    if (message->type() == Message::Type::Event)
    {
        for (const Socket::Ptr& s : sockets)
            if (!excludeSockets.contains(s->socketDescriptor()))
                s->send(message);
    }
    else
    {
        if (!message->destinationSocketDescriptors().isEmpty())
        {
            bool messageSended = false;
            for (const Socket::Ptr& s : sockets)
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
            for (const Socket::Ptr& s : sockets)
                if (s->socketDescriptor() == message->socketDescriptor())
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

void Listener::send(const Message::Ptr& message, SocketDescriptor excludeSocket) const
{
    SocketDescriptorSet excludeSockets;
    excludeSockets << excludeSocket;
    send(message, excludeSockets);
}

Socket::Ptr Listener::socketByDescriptor(SocketDescriptor descr) const
{
    QVector<Socket::Ptr> sockets = this->sockets();
    for (const Socket::Ptr& s : sockets)
        if (s->socketDescriptor() == descr)
            return s;

    return Socket::Ptr();
}

void Listener::incomingConnection (SocketDescriptor socketDescriptor)
{
    Socket::Ptr socket = Socket::Ptr::create_ptr();
    socket->setSocketDescriptor(socketDescriptor);
    socket->setCompressionLevel(_compressionLevel);
    socket->setCompressionSize(_compressionSize);
    socket->setCheckProtocolCompatibility(_checkProtocolCompatibility);
    socket->setCheckUnknownCommands(_checkUnknownCommands);
    //socket->setEemitMessageRaw(_emitMessageRaw);

    chk_connect_d(socket.get(), SIGNAL(message(communication::Message::Ptr)),
                  this, SIGNAL(message(communication::Message::Ptr)))

    chk_connect_d(socket.get(), SIGNAL(connected(communication::SocketDescriptor)),
                  this, SIGNAL(socketConnected(communication::SocketDescriptor)))

    chk_connect_d(socket.get(), SIGNAL(disconnected(communication::SocketDescriptor)),
                  this, SIGNAL(socketDisconnected(communication::SocketDescriptor)))

    // Примечание: выход из функции start() происходит только тогда, когда
    // поток гарантированно запустится, поэтому случайное удаление нового
    // сокета в функции removeClosedSockets() исключено.
    socket->start();

    QMutexLocker locker(&_socketsLock); (void) locker;
    _sockets.append(socket);
}

void Listener::removeClosedSockets()
{
    QMutexLocker locker(&_socketsLock); (void) locker;
    for (int i = 0; i < _sockets.size(); ++i)
    {
        Socket::Ptr s = _sockets[i];
        if (!s->isRunning())
        {
            _sockets.remove(i--);
            break;
        }
    }
}

} // namespace tcp
} // namespace transport

transport::tcp::Listener& listener()
{
    return ::safe_singleton<transport::tcp::Listener>();
}

} // namespace communication

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
