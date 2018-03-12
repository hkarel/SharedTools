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
    registrationQtMetatypes();
}

bool Socket::init(const HostPoint& bindPoint)
{
    if (isRunning())
    {
        log_error_m << "Impossible execute a initialization because Sender thread is running.";
        return false;
    }
    _bindPoint = bindPoint;
    return true;
}

void Socket::waitBinding(int time)
{
    if ((time <= 0) || isBound())
        return;

    QElapsedTimer timer;
    timer.start();
    while (!timer.hasExpired(time * 1000))
    {
        if (threadStop())
            break;
        msleep(100);
        if (isBound())
            break;
    }
}

bool Socket::isBound() const
{
    SpinLocker locker(_socketLock); (void) locker;
    return (_socket && (_socket->state() == QAbstractSocket::BoundState));
}

SocketDescriptor Socket::socketDescriptor() const
{
    SpinLocker locker(_socketLock); (void) locker;
    return (_socket) ? _socket->socketDescriptor() : -1;
}

QList<QHostAddress> Socket::discardAddresses() const
{
    SpinLocker locker(_discardAddressesLock); (void) locker;
    return _discardAddresses;
}

void Socket::setDiscardAddresses(const QList<QHostAddress>& val)
{
    SpinLocker locker(_discardAddressesLock); (void) locker;
    _discardAddresses = val;
}

void Socket::run()
{
    { //Block for SpinLocker
        SpinLocker locker(_socketLock); (void) locker;
        _socket = simple_ptr<QUdpSocket>(new QUdpSocket(0));
    }
    if (!_socket->bind(_bindPoint.address(), _bindPoint.port(),
                       QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint))
    {
        log_error_m << "Failed bind UDP socket"
                    << ". Error code: " << int(_socket->error())
                    << ". Detail: " << _socket->errorString();
        return;
    }
    log_debug_m << "UDP socket is successfully bound to point " << _bindPoint;

    Message::List internalMessages;
    Message::List acceptMessages;

    QElapsedTimer timer;
    bool loopBreak = false;
    const int delay = 50;

    #define CHECK_SOCKET_ERROR \
        if (_socket->state() != QAbstractSocket::BoundState) \
        { \
            log_error_m << "UDP socket error code: " << int(_socket->error()) \
                        << ". Detail: " << _socket->errorString(); \
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

            while (!_socket->hasPendingDatagrams()
                   && _messagesCount == 0
                   && acceptMessages.empty())
            {
                if (threadStop())
                {
                    loopBreak = true;
                    break;
                }
                QMutexLocker locker(&_messagesLock); (void) locker;
                if (_messagesCount != 0)
                    break;
                _messagesCond.wait(&_messagesLock, 20);
            }
            if (loopBreak)
                break;

            QList<QHostAddress> discardAddresses;
            { //Block for SpinLocker
                SpinLocker locker(_discardAddressesLock); (void) locker;
                discardAddresses = _discardAddresses;
            }

            //--- Отправка сообщений ---
            timer.start();
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
                                 << "; message id: " << message->id()
                                 << "; command: " << CommandNameLog(message->command());
                }
                if (message->size() > 500)
                    log_warn_m << "Too large message to send it through a UDP socket"
                               << ". The message may be lost"
                               << ". Command: " << CommandNameLog(message->command());

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
                        _socket->writeDatagram(buff, dp.address(), dp.port());

                    CHECK_SOCKET_ERROR
                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        alog::Line logLine =
                            log_debug2_m << "Message was sent to the next addresses:";
                        for (const HostPoint& dp : message->destinationPoints())
                            logLine << " " << dp;
                        logLine << "; message id: " << message->id();
                        logLine << "; command: " << CommandNameLog(message->command());
                    }
                }
                else if (!message->sourcePoint().isNull())
                {
                    _socket->writeDatagram(buff,
                                           message->sourcePoint().address(),
                                           message->sourcePoint().port());
                    CHECK_SOCKET_ERROR
                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        log_debug2_m << "Message was sent to the address"
                                     << ": " << message->sourcePoint()
                                     << "; message id: " << message->id()
                                     << "; command: " << CommandNameLog(message->command());
                    }
                }
                else
                {
                    log_error_m << "Impossible send message: " << CommandNameLog(message->command())
                                << "; message id: " << message->id()
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
            timer.start();
            while (_socket->hasPendingDatagrams())
            {
                if (loopBreak
                    || timer.hasExpired(3 * delay))
                    break;

                if (_socket->pendingDatagramSize() < qint64(sizeof(udpSignature)))
                    continue;

                QByteArray datagram;
                QHostAddress addr;
                quint16 port;
                qint64 datagramSize = _socket->pendingDatagramSize();
                datagram.resize(datagramSize);
                qint64 res = _socket->readDatagram((char*)datagram.constData(),
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
                if (discardAddresses.contains(addr) && (port == _bindPoint.port()))
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
                                 << "; message id: " << message->id()
                                 << "; command: " << CommandNameLog(message->command())
                                 << "; source: " << addr << ":" << port;
                }
                message->setSocketType(SocketType::Udp);
                message->setSocketDescriptor(SocketDescriptor(-1));
                message->setSourcePoint({addr, port});
                acceptMessages.add(message.detach());
                CHECK_SOCKET_ERROR
            }
            if (loopBreak)
                break;

            //--- Обработка принятых сообщений ---
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
                                        << "; remote host:" << unknown.address << ":" << unknown.port
                                        << "; socket descriptor: " << unknown.socketDescriptor;

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
                        unknown.address = _socket->localAddress();
                        unknown.port = _socket->localPort();
                        unknown.socketDescriptor = _socket->socketDescriptor();
                        Message::Ptr mUnknown = createMessage(unknown);
                        mUnknown->setPriority(Message::Priority::High);
                        internalMessages.add(mUnknown.detach());
                        log_error_m << "Unknown command: " << unknown.commandId
                                    << "; host: " << unknown.address << ":" << unknown.port
                                    << "; socket descriptor: " << unknown.socketDescriptor;
                        continue;
                    }
                }

                try
                {
                    if (alog::logger().level() == alog::Level::Debug2)
                    {
                        log_debug2_m << "Message emit"
                                     << "; message id: " << m->id()
                                     << "; command: " << CommandNameLog(m->command());
                    }
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
