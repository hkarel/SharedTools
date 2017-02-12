#include "break_point.h"
#include "spin_locker.h"
#include "safe_singleton.h"
#include "logger/logger.h"
#include "qt/logger/logger_operators.h"
#include "qt/version/version_number.h"
#include "qt/communication/commands_base.h"
#include "qt/communication/commands_pool.h"
#include "qt/communication/transport/udp.h"
#include "qt/communication/logger_operators.h"

#include <stdexcept>

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportUDP")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportUDP")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportUDP")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportUDP")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportUDP")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "TransportUDP")


namespace {
// Сигнатура для UDP протокола 'GMA7'
//const quint32 udpSignature = 0x37414D47;
const quint32 udpSignature = *((quint32*)UDP_SIGNATURE);
}

namespace communication {
namespace transport {
namespace udp {

Socket::Socket()
{
    static bool first {true};
    if (first)
    {
        qRegisterMetaType<communication::Message::Ptr>("communication::Message::Ptr");
        first = false;
    }
}

bool Socket::init(const QHostAddress& address, int port)
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

    if (!_socket.bind(_address, _port,
                      QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint))
    {
        log_error_m << "Failed bind UDP socket"
                    << "; Error code: " << int(_socket.error())
                    << "; Detail: " << _socket.errorString();
        return false;
    }
    log_debug_m << "UDP socket is successfully bound"
                << ". Address: " << _address.toString()
                << " Port: " << _port;

    return true;
}

NetAddressesPtr Socket::discardAddresses() const
{
    SpinLocker locker(_discardAddressesLock); (void) locker;
    return _discardAddresses;
}

void Socket::setDiscardAddresses(const NetAddressesPtr& val)
{
    SpinLocker locker(_discardAddressesLock); (void) locker;
    _discardAddresses = val;
}

bool Socket::send(const Message::Ptr& message)
{
    if (!isRunning())
    {
        log_error_m << "Sender is not active"
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

SocketDescriptor Socket::socketDescriptor() const
{
    return _socket.socketDescriptor();
}

void Socket::run()
{
    Message::List internalMessages;
    Message::List acceptMessages;

    QElapsedTimer timer;
    bool loopBreak = false;
    const int delay = 50;

    auto socketIsActual = [this]() -> bool
    {
        //return (this->_socket->isValid()
        //        && (this->_socket->state() == QAbstractSocket::BoundState));
        return (this->_socket.state() == QAbstractSocket::BoundState);
    };

    #define CHECK_SOCKET_ERROR \
        if (!socketIsActual()) \
        { \
            log_error_m << "UDP socket error code: " << int(_socket.error()) \
                       << "; Detail: " << _socket.errorString(); \
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

            { //Block for QMutexLocker
                QMutexLocker locker(&_messagesLock); (void) locker;
                _messagesCount = _messagesHigh.count()
                                 + _messagesNorm.count()
                                 + _messagesLow.count();
            }

            while (!_socket.hasPendingDatagrams()
                   && _messagesCount == 0
                   && acceptMessages.empty())
            {
                if (threadStop())
                {
                    loopBreak = true;
                    break;
                }
                msleep(20);
            }
            if (loopBreak)
                break;

            NetAddressesPtr discardAddresses;
            { //Block for SpinLocker
                SpinLocker locker(_discardAddressesLock); (void) locker;
                discardAddresses = _discardAddresses;
            }

            //--- Отправка сообщений ---
            timer.restart();
            while (true)
            {
                Message::Ptr message;
                if (!internalMessages.empty())
                    message.attach(internalMessages.release(0));

                if (message.empty()
                    && _messagesCount != 0)
                { //Block for QMutexLocker
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
                    log_debug2_m << "Message before sending to the UDP socket"
                                 << ". Command " << CommandNameLog(message->command());
                }
                if (message->size() > 500)
                    log_warn_m << "Too large message to send it through a UDP socket"
                               << ". The message may be lost"
                               << ". Command " << CommandNameLog(message->command());

                //break_point
                // Проверить значение udpSignature
                //quint32 udpSignature__ = qbswap(udpSignature);
                //quint32 udpSignature__ = udpSignature;

                QByteArray buff;
                buff.reserve(message->size() + sizeof(udpSignature));
                {
                    QDataStream stream {&buff, QIODevice::WriteOnly};
                    stream << udpSignature;
                    message->toDataStream(stream);
                }

                if (!message->destinationPoints().isEmpty())
                {
                    for (const HostPoint& dp : message->destinationPoints())
                        _socket.writeDatagram(buff, dp.address, dp.port);

                    CHECK_SOCKET_ERROR
                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        alog::Line logLine =
                            log_debug2_m << "Message was sent to the next addresses:";
                        for (const HostPoint& dp : message->destinationPoints())
                            logLine << " " << dp.address.toString() << ":" << dp.port;
                        logLine << "; Command " << CommandNameLog(message->command());
                    }
                }
                else if (!message->sourcePoint().isNull())
                {
                    _socket.writeDatagram(buff,
                                          message->sourcePoint().address,
                                          message->sourcePoint().port);
                    CHECK_SOCKET_ERROR
                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        log_debug2_m << "Message was sent to the address:"
                                     << " " << message->sourcePoint().address
                                     << ":" << message->sourcePoint().port
                                     << "; Command " << CommandNameLog(message->command());
                    }
                }
                else
                {
                    log_error_m << "Impossible send message: " << CommandNameLog(message->command())
                                << ". Destination host point is undefined"
                                << ". Message will be discarded";
                }
                if (loopBreak
                    || timer.hasExpired(3 * delay))
                    break;
            }
            if (loopBreak)
                break;

            //--- Прием сообщений ---
            timer.restart();
            while (_socket.hasPendingDatagrams())
            {
                if (loopBreak
                    || timer.hasExpired(3 * delay))
                    break;

                if (_socket.pendingDatagramSize() < qint64(sizeof(udpSignature)))
                    continue;

                QByteArray datagram;
                QHostAddress addr;
                quint16 port;
                qint64 datagramSize = _socket.pendingDatagramSize();
                datagram.resize(datagramSize);
                qint64 res = _socket.readDatagram((char*)datagram.constData(),
                                                  datagramSize, &addr, &port);
                if (res != datagramSize)
                {
                    log_error_m << "Failed datagram size"
                                << ". Source: " << addr << ":" << port;
                    continue;
                }
                if (alog::logger().level() == alog::Level::Debug2)
                {
                    log_debug2_m << "Raw message received"
                                 << ". Source: " << addr << ":" << port;
                }
                if (discardAddresses
                    && discardAddresses->contains(addr)
                    && port == _port)
                {
                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        log_debug2_m << "Raw message discarded"
                                     << ". Source: " << addr << ":" << port;
                    }
                    continue;
                }

                Message::Ptr message;
                { //Block for QDataStream
                    QDataStream stream {&datagram, QIODevice::ReadOnly | QIODevice::Unbuffered};
                    quint32 udpSign;
                    stream >> udpSign;
                    if (udpSign != udpSignature)
                    {
                        if (alog::logger().level() == alog::Level::Debug2)
                        {
                            log_debug2_m << "Raw message incompatible signature, discarded"
                                         << ". Source: " << addr << ":" << port;
                        }
                        continue;
                    }
                    message = Message::fromDataStream(stream);
                }
                if (alog::logger().level() == alog::Level::Debug2)
                {
                    log_debug2_m << "Message received"
                                 << ". Command " << CommandNameLog(message->command())
                                 << ". Source: " << addr << ":" << port;
                }
                //message->setSocketDescriptor(_socket->socketDescriptor());
                message->setSourcePoint({addr, port});
                acceptMessages.add(message.detach());
                CHECK_SOCKET_ERROR
            }
            if (loopBreak)
                break;

            //--- Обработка принятых сообщений ---
            timer.restart();
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
                        break_point
                        // Отладить

                        data::Unknown unknown;
                        unknown.commandId = m->command();
                        unknown.address = _socket.localAddress();
                        unknown.port = _socket.localPort();
                        unknown.socketDescriptor = _socket.socketDescriptor();
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
        } // while (true)

        _socket.close();
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

Socket& socket()
{
    return ::safe_singleton<Socket>();
}

} // namespace udp
} // namespace transport
} // namespace communication

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
