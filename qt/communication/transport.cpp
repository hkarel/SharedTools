#include "transport.h"
#include "commands_base.h"
#include "commands_pool.h"
#include "break_point.h"
#include "logger/logger.h"
#include "qt/logger/logger_operators.h"
#include "qt/version/version_number.h"

#include <stdexcept>

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Communication")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Communication")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Communication")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "Communication")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Communication")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Communication")

/**
  Вспомогательная структура, используется для отправки в лог идентификатора
  команды вместе с именем.
*/
namespace {
struct CommandNameLog
{
    const QUuidEx& command;
    CommandNameLog(const QUuidEx& command) : command(command) {}
};
} // namespace

namespace alog {
Line& operator<< (Line& line, const CommandNameLog& cnl)
{
    if (line.toLogger())
    {
        QByteArray commandName = communication::commandsPool().commandName(cnl.command);
        if (!commandName.isEmpty())
            line << commandName << "/";
        line << cnl.command;
    }
    return line;
}

Line operator<< (Line&& line, const CommandNameLog& cnl)
{
    if (line.toLogger())
    {
        QByteArray commandName = communication::commandsPool().commandName(cnl.command);
        if (!commandName.isEmpty())
            line << commandName << "/";
        line << cnl.command;
    }
    return std::move(line);
}
} // namespace alog


namespace communication {
namespace transport {

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
        log_error_m << "Socket is not active. Command "
                    << CommandNameLog(message->command()) << " will be discarded";
        return false;
    }
    if (message.empty())
    {
        log_error_m << "Cannot send a empty message";
        return false;
    }
    if (!remoteCommandExists(message->command()))
    {
        log_error_m << "Command " << CommandNameLog(message->command())
                    << " is unknown for the receiving side. Command will be discarded.";
        return false;
    }
    { //Block for SpinLocker
        SpinLocker locker(_messagesLock); (void) locker;
        message->add_ref();
        _messages.add(message.get());

    }
    if (alog::logger().level() == alog::Level::Debug2)
    {
        log_debug2_m << "Message added to a queue to sending"
                     << ". Command " << CommandNameLog(message->command());
    }
    _loopCondition.wakeAll();
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
    SpinLocker locker(_removeMessagesLock); (void) locker;
    _removeMessages.insert(command);
}

Socket::BinaryProtocol Socket::binaryProtocolStatus() const
{
    return _binaryProtocolStatus;
}

SocketDescriptor Socket::socketDescriptor() const
{
    return (_socket) ? _socket->socketDescriptor() : -1;
}

bool Socket::remoteCommandExists(const QUuidEx& command) const
{
    SpinLocker locker(_remoteCommandsLock); (void) locker;
    return (_remoteCommands.constFind(command) != _remoteCommands.constEnd());
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

    Message::List sendMessages;
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
        data::CompatibleInfo compatibleInfo;
        compatibleInfo.version = productVersion();
        compatibleInfo.minCompatibleVersion = minCompatibleVersion();
        compatibleInfo.commands = commandsPool().commands();
        Message::Ptr message = createMessage(compatibleInfo);
        sendMessages.add(message.detach());
    }

    auto processingCompatibleInfoCommand = [&](Message::Ptr& message) -> void
    {
        if (message->commandType() == command::Type::Request)
        {
            VersionNumber productVersion = ::productVersion();
            VersionNumber minCompatibleVersion = ::minCompatibleVersion();
            VersionNumber remoteProductVersion;
            VersionNumber remoteMinCompatibleVersion;

            data::CompatibleInfo compatibleInfo;
            readFromMessage(message, compatibleInfo);
            if (compatibleInfo.isValid)
            {
                // Получаем список команд, которые доступны противоположной стороне
                QSet<QUuidEx> remoteCommands;
                for (const QUuidEx& command : compatibleInfo.commands)
                    remoteCommands.insert(command);

                { //Block for SpinLocker
                    SpinLocker locker(_remoteCommandsLock); (void) locker;
                    _remoteCommands.swap(remoteCommands);
                }

                remoteProductVersion = compatibleInfo.version;
                remoteMinCompatibleVersion = compatibleInfo.minCompatibleVersion;

                if (alog::logger().level() >= alog::Level::Debug)
                {
                    log_debug_m
                        << "Compatible info"
                        << ". Product version: " << productVersion.toString()
                        << ", min compatible version: " << minCompatibleVersion.toString()
                        << "; Remote product version: " << remoteProductVersion.toString()
                        << ", remote min compatible version: " << remoteMinCompatibleVersion.toString();
                }

                _binaryProtocolStatus = BinaryProtocol::Compatible;
                if (remoteProductVersion < minCompatibleVersion)
                    _binaryProtocolStatus = BinaryProtocol::Incompatible;
            }
            else
            {
                log_error_m << "Incorrect data structure for command "
                            << CommandNameLog(message->command());
                _binaryProtocolStatus = BinaryProtocol::Incompatible;
            }

            if (_binaryProtocolStatus == BinaryProtocol::Compatible)
            {
                emit connected(_socket->socketDescriptor());
            }
            else // BinaryProtocol::Incompatible
            {
                data::CloseConnection closeConnection;
                closeConnection.description = QString(
                    "Binary protocol versions incompatible"
                    ". Product version: %1, min compatible version: %2"
                    "; Remote product version: %3, remote min compatible version: %4"
                )
                .arg(productVersion.toString()).arg(minCompatibleVersion.toString())
                .arg(remoteProductVersion.toString()).arg(remoteMinCompatibleVersion.toString());

                log_verbose_m << "Send request to close the connection"
                              << "; Detail: " << closeConnection.description;

                Message::Ptr m = createMessage(closeConnection);
                commandCloseConnectionId = m->id();
                sendMessages.add(m.detach());
            }
        }
    };

    auto processingCloseConnectionCommand = [&](Message::Ptr& message) -> void
    {
        if (message->commandType() == command::Type::Request)
        {
            data::CloseConnection closeConnection;
            readFromMessage(message, closeConnection);
            if (closeConnection.isValid)
            {
                log_verbose_m << "Connection will be closed at the request remote side"
                              << "; Remote detail: " << closeConnection.description;
            }
            else
                log_error_m << "Incorrect data structure for command "
                            << CommandNameLog(message->command());

            // Отправляем ответ
            message->clearContent();
            message->setCommandType(command::Type::Response);
            message->setCommandExecStatus(command::ExecStatus::Success);
            message->add_ref();
            sendMessages.add(message.get());
        }
        else if (message->commandType() == command::Type::Response
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
                _protocolSignatureWrite = true;
            }

            // Проверка сигнатуры протокола
            if (!_protocolSignatureRead)
            {
                timer.restart();
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

            // Отправляем внешние сообщения только когда есть ясность
            // по совместимости бинарных протоколов.
            if (_binaryProtocolStatus == BinaryProtocol::Compatible)
            {
                Message::List messages;
                { //Block for SpinLocker
                    SpinLocker locker(_messagesLock); (void) locker;
                    messages.swap(_messages);
                }
                for (int i = 0; i < messages.count(); ++i)
                    sendMessages.add(messages.release(i, lst::NO_COMPRESS_LIST));
                messages.clear();
            }

            // Удаление сообщений из очереди
            QSet<QUuidEx> removeMessages;
            { //Block for SpinLocker
                SpinLocker locker(_removeMessagesLock); (void) locker;
                removeMessages.swap(_removeMessages);
            }
            if (!removeMessages.isEmpty())
            {
                sendMessages.removeCond(
                    [&removeMessages](Message* m) -> bool
                    {
                        bool res = removeMessages.contains(m->command());
                        if (res && (alog::logger().level() == alog::Level::Debug2))
                            log_debug2_m << "Message removed to a queue to sending."
                                         << " Command " << CommandNameLog(m->command());
                        return res;
                    });
                removeMessages.clear();
            }

            _sendMessagesCount = sendMessages.count();

            if (_socket->bytesAvailable() == 0
                &&_socket->bytesToWrite() == 0
                && sendMessages.empty()
                && acceptMessages.empty())
            {
                //msleep(250);
                { //Block for QMutexLocker
                    QMutexLocker locker(&_loopConditionLock); (void) locker;
                    _loopCondition.wait(&_loopConditionLock, 500);
                }
                // Нет ясности, как реализованы методы waitForBytesWritten(),
                // waitForReadyRead(), возможно они отвечают за изменение значений
                // для bytesAvailable() и bytesToWrite(), поэтому здесь их вызываем,
                // чтобы не попасть в мертвый цикл.
                _socket->waitForBytesWritten(delay);
                _socket->waitForReadyRead(delay);
                continue;
            }

            timer.restart();
            while (_socket->bytesToWrite())
            {
                _socket->waitForBytesWritten(delay);
                CHECK_SOCKET_ERROR
                if (timer.hasExpired(3 * delay))
                    break;
            }
            if (loopBreak)
                break;

            //--- Отправка сообщений ---
            if (_socket->bytesToWrite() == 0)
            {
                timer.restart();
                while (!sendMessages.empty())
                {
                    Message::Ptr message {sendMessages.release(0), false};
                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        log_debug2_m << "Message before sending to the socket."
                                     << " Command " << CommandNameLog(message->command());
                    }
                    QByteArray buff = message->toByteArray();
                    qint32 buffSize = buff.size();

                    if (!isLoopback()
                        &&(_compressionLevel != 0)
                        && (buff.size() > _compressionSize))
                    {
                        qint32 buffSizePrev = buffSize;
                        buff = qCompress(buff, _compressionLevel);
                        buffSize = buff.size();
                        if (alog::logger().level() == alog::Level::Debug2)
                        {
                            log_debug2_m << "Message was compressed."
                                         << " Command " << CommandNameLog(message->command())
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
                        &&_socket->bytesToWrite() == 0
                        && timer.hasExpired(3 * delay))
                    {
                        log_debug2_m << "Message was send to the socket."
                                     << " Command " << CommandNameLog(message->command());
                    }
                    if (loopBreak
                        || timer.hasExpired(3 * delay))
                        break;
                }
                if (loopBreak)
                    break;
            } //--- Отправка сообщений ---

            if (_socket->bytesAvailable() == 0)
                _socket->waitForReadyRead(3 * delay);

            //--- Прием сообщений ---
            if (_socket->bytesAvailable() != 0)
            {
                timer.restart();
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

                        if (alog::logger().level() == alog::Level::Debug2)
                        {
                            log_debug2_m << "Message received. Command "
                                         << CommandNameLog(message->command());
                        }

                        if (message->command() == command::Unknown)
                        {
                            // Обработка уведомления о неизвестной команде
                            data::Unknown unknown;
                            readFromMessage(message, unknown);
                            if (unknown.isValid)
                            {
                                log_error_m << "Command " << CommandNameLog(unknown.commandId)
                                            << " is unknown for the remote side"
                                            << "; Remote host:" << unknown.address << ":" << unknown.port
                                            << "; Socket descriptor: " << unknown.socketDescriptor;
                            }
                            else
                                log_error_m << "Incorrect data structure for command "
                                            << CommandNameLog(message->command());
                        }
                        else if (_binaryProtocolStatus == BinaryProtocol::Undefined
                                 && message->command() == command::CompatibleInfo)
                        {
                            processingCompatibleInfoCommand(message);
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
                            // Отправляем сообщения в очередь на обработку только
                            // когда есть ясность по совместимости бинарных прото-
                            // колов, в противном случае все сообщения обрабаты-
                            // ваются как внутренние.
                            if (_binaryProtocolStatus == BinaryProtocol::Compatible)
                            {
                                message->add_ref();
                                acceptMessages.add(message.get());
                            }
                            else
                            {
                                log_error_m
                                    << "Check of compatibility for the binary protocol not performed"
                                    << " Command " << CommandNameLog(message->command()) << " discarded";
                            }
                        }
                    }
                    if (loopBreak
                        || timer.hasExpired(3 * delay))
                        break;
                } // while (true)
                if (loopBreak)
                    break;
            } //--- Прием сообщений ---

            //--- Обработка принятых сообщений ---
            if (_binaryProtocolStatus == BinaryProtocol::Compatible)
            {
                timer.restart();
                while (!acceptMessages.empty())
                {
                    Message::Ptr m {acceptMessages.release(0), false};

                    // Если команда неизвестна - отправляем об этом уведомление
                    // и переходим к обработке следующей команды.
                    if (!commandsPool().commandExists(m->command()))
                    {
                        data::Unknown unknown;
                        unknown.commandId = m->command();
                        unknown.address = _socket->peerAddress();
                        unknown.port = _socket->peerPort();
                        unknown.socketDescriptor = _socket->socketDescriptor();
                        Message::Ptr mu = createMessage(unknown);
                        sendMessages.add(mu.detach());
                        log_error_m << "Unknown command: " << unknown.commandId
                                    << "; Host: " << unknown.address << ":" << unknown.port
                                    << "; Socket descriptor: " << unknown.socketDescriptor;
                        continue;
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
    static bool first {true};
    if (first)
    {
        qRegisterMetaType<communication::Message::Ptr>("communication::Message::Ptr");
        qRegisterMetaType<communication::SocketDescriptor>("communication::SocketDescriptor");
        first = false;
    }
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
        if (!_socket->waitForConnected(10 * 1000 /*ждем 10 сек*/))
        {
            log_error_m << "Failed connect to host "
                        << _address << ":" << _port
                        << "; Error code: " << int(_socket->error())
                        << "; Detail: " << _socket->errorString();
            sleep(10);
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
    static bool first {true};
    if (first)
    {
        qRegisterMetaType<communication::Message::Ptr>("communication::Message::Ptr");
        qRegisterMetaType<communication::SocketDescriptor>("communication::SocketDescriptor");
        first = false;
    }

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
    if (message->commandType() == command::Type::Event)
    {
        for (const Socket::Ptr& s : sockets)
            if (!excludeSockets.contains(s->socketDescriptor()))
                s->send(message);
    }
    else
    {
        for (const Socket::Ptr& s : sockets)
            if (s->socketDescriptor() == message->socketDescriptor())
            {
                s->send(message);
                return;
            }
        log_error_m << "Not found socket with descriptor: " << message->socketDescriptor()
                    << ". Message with command id: " << CommandNameLog(message->command())
                    << " will be discarded";
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
    Socket::Ptr socket = Socket::Ptr::create_join_ptr();
    socket->setSocketDescriptor(socketDescriptor);
    socket->setCompressionLevel(_compressionLevel);
    socket->setCompressionSize(_compressionSize);

    chk_connect_d(socket.get(), SIGNAL(message(communication::Message::Ptr)),
                  this, SIGNAL(message(communication::Message::Ptr)))

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

} // namespace transport

transport::Listener& listener()
{
    return ::safe_singleton<transport::Listener>();
}

} // namespace communication

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
