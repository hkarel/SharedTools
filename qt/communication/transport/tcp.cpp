/*****************************************************************************
  The MIT License

  Copyright © 2015, 2017 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "qt/communication/transport/tcp.h"

#include "break_point.h"
#include "logger/logger.h"
#include "logger/format.h"
#include "qt/logger/logger_operators.h"
#include "qt/communication/logger_operators.h"
#include "qt/communication/utils.h"

#ifdef JSON_SERIALIZE
#include "qt/communication/serialize/json.h"
#endif

#include <stdexcept>
#include <unistd.h>

#define log_error_m   alog::logger().error  (__FILE__, __func__, __LINE__, "TransportTCP")
#define log_warn_m    alog::logger().warn   (__FILE__, __func__, __LINE__, "TransportTCP")
#define log_info_m    alog::logger().info   (__FILE__, __func__, __LINE__, "TransportTCP")
#define log_verbose_m alog::logger().verbose(__FILE__, __func__, __LINE__, "TransportTCP")
#define log_debug_m   alog::logger().debug  (__FILE__, __func__, __LINE__, "TransportTCP")
#define log_debug2_m  alog::logger().debug2 (__FILE__, __func__, __LINE__, "TransportTCP")

namespace communication {
namespace transport {
namespace tcp {

//---------------------------------- Socket ----------------------------------

void printHostInfo(alog::Line& logLine, const QString& name, const HostPoint& peerPoint)
{
    if (!name.isEmpty())
        logLine << log_format(" '?'. Host: ?", name, peerPoint);
    else
        logLine << log_format(" host: ?", peerPoint);
}

bool Socket::init(const HostPoint& peerPoint)
{
    if (isRunning())
    {
        log_error_m << "Impossible execute a initialization because Socket thread is running";
        return false;
    }
    _peerPoint = peerPoint;
    return true;
}

void Socket::socketCreate()
{
    _socket = simple_ptr<QTcpSocket>(new QTcpSocket(0));
    chk_connect_d(_socket.get(), SIGNAL(disconnected()), this, SLOT(socketDisconnected()))
}

bool Socket::socketInit()
{
    const char* connectDirection;
    if (initSocketDescriptor() == -1)
    {
        { //Block for alog::Line
            alog::Line logLine = log_verbose_m << "Try connect to";
            printHostInfo(logLine, name(), _peerPoint);
        }

        connectDirection = "to";
        _socket->connectToHost(_peerPoint.address(), _peerPoint.port());
        if (!_socket->waitForConnected(3*1000 /*3 сек*/))
        {
            alog::Line logLine = log_error_m << "Failed connect to";
            printHostInfo(logLine, name(), _peerPoint);
            logLine << ". Error code: " << int(_socket->error())
                    << ". Detail: " << _socket->errorString();
            return false;
        }
    }
    else
    {
        connectDirection = "from";
        if (!_socket->setSocketDescriptor(initSocketDescriptor()))
        {
            alog::Line logLine = log_error_m << "Failed set socket descriptor";
            printHostInfo(logLine, name(), _peerPoint);
            logLine << ". Error code: " << int(_socket->error())
                    << ". Detail: " << _socket->errorString();
            return false;
        }
    }

    try
    {
        _peerPoint.setAddress(_socket->peerAddress());
        _peerPoint.setPort(_socket->peerPort());
        _printSocketDescriptor = _socket->socketDescriptor();
    }
    catch (std::exception& e)
    {
        alog::Line logLine = log_error_m << "Failed socket init";
        printHostInfo(logLine, name(), _peerPoint);
        logLine << ". Detail: " << e.what();
        return false;
    }
    catch (...)
    {
        alog::Line logLine = log_error_m << "Failed socket init";
        printHostInfo(logLine, name(), _peerPoint);
        logLine << ". Unknown error";
        return false;
    }

    { //Block for alog::Line
        alog::Line logLine = log_verbose_m << "Connect " << connectDirection;
        printHostInfo(logLine, name(), _peerPoint);
        logLine << ". Socket descriptor: " << _printSocketDescriptor;
    }
    return true;
}

bool Socket::isLocalInternal() const
{
#if QT_VERSION >= 0x050000
    return (_socket && _socket->peerAddress().isLoopback());
#else
    return (_socket && (_socket->peerAddress() == QHostAddress::LocalHost));
#endif
}

SocketDescriptor Socket::socketDescriptorInternal() const
{
    return (_socket) ? _socket->socketDescriptor() : -1;
}

bool Socket::socketIsConnectedInternal() const
{
    return (_socket
            && _socket->isValid()
            && _socket->state() == QAbstractSocket::ConnectedState);
}

void Socket::printSocketError(const char* file, const char* func, int line,
                              const char* module)
{
    if (_socket->error() == QAbstractSocket::RemoteHostClosedError)
    {
        alog::Line logLine =
            alog::logger().verbose(file, func, line, "TransportTCP")
            << _socket->errorString()
            << ". Remote host: " << _peerPoint;

        if (!name().isEmpty())
            logLine << ". Socket name: " << name();

        logLine << ". Socket descriptor: " << _printSocketDescriptor;
    }
    else
    {
        alog::Line logLine =
            alog::logger().error(file, func, line, module)
            << "Socket error code: " << int(_socket->error());

        if (!name().isEmpty())
            logLine << ". Socket name: " << name();

        logLine << ". Detail: " << _socket->errorString();
    }
}

qint64 Socket::socketBytesAvailable() const
{
    return _socket->bytesAvailable();
}

qint64 Socket::socketBytesToWrite() const
{
    return _socket->bytesToWrite();
}

qint64 Socket::socketRead(char* data, qint64 maxlen)
{
    return _socket->read(data, maxlen);
}

qint64 Socket::socketWrite(const char* data, qint64 len)
{
    return _socket->write(data, len);
}

bool Socket::socketWaitForReadyRead(int msecs)
{
    return _socket->waitForReadyRead(msecs);
}

bool Socket::socketWaitForBytesWritten(int msecs)
{
    return _socket->waitForBytesWritten(msecs);
}

void Socket::socketClose()
{
    try
    {
        if (_socket->isValid()
            && _socket->state() != QAbstractSocket::UnconnectedState)
        {
            alog::Line logLine = log_verbose_m << "Disconnected from";
            printHostInfo(logLine, name(), _peerPoint);
            logLine << ". Socket descriptor: " << _socket->socketDescriptor();

            _socket->disconnectFromHost();
            if (_socket->state() != QAbstractSocket::UnconnectedState)
                _socket->waitForDisconnected(1000);
        }
        _socket->close();
    }
    catch (std::exception& e)
    {
        log_error_m << "Detail: " << e.what();
        alog::logger().flush();
        alog::logger().waitingFlush();
    }
    catch (...)
    {
        log_error_m << "Unknown error";
        alog::logger().flush();
        alog::logger().waitingFlush();
    }
    _socket.reset();
}

void Socket::messageInit(Message::Ptr& message)
{
    message->setSocketType(SocketType::Tcp);
    message->setSocketDescriptor(_socket->socketDescriptor());
    message->setSourcePoint({_socket->peerAddress(), _socket->peerPort()});
}

void Socket::fillUnknownMessage(const Message::Ptr& message, data::Unknown& unknown)
{
    unknown.commandId = message->command();
    unknown.socketType = SocketType::Tcp;
    unknown.socketDescriptor = _socket->socketDescriptor();
    unknown.socketName.clear();
    unknown.address = _socket->peerAddress();
    unknown.port = _socket->peerPort();
}

//--------------------------------- Listener ---------------------------------

Listener::Listener()
{
    registrationQtMetatypes();
    chk_connect_q(&_removeClosedSockets, SIGNAL(timeout()),
                  this, SLOT(removeClosedSockets()))
}

bool Listener::init(const HostPoint& listenPoint)
{
    _listenPoint = listenPoint;
    int attempt = 0;
    while (!QTcpServer::listen(_listenPoint.address(), _listenPoint.port()))
    {
        if (++attempt > 10)
            break;
        usleep(200*1000);
    }
    if (attempt > 10)
    {
        alog::Line logLine = log_error_m << "Start listener is failed";
        if (!name().isEmpty())
            logLine << log_format(". Listener name: '?'", name());

        logLine << ". Connection point: " << _listenPoint
                << ". Detail: " << errorString();
    }
    else
    {
        alog::Line logLine = log_verbose_m << "Start listener";
        if (!name().isEmpty())
            logLine << log_format(" '?'", name());

        logLine << ". Connection point: " << serverAddress() << ":" << serverPort();
    }

    _removeClosedSockets.start(15*1000);
    return (attempt <= 10);
}

void Listener::close()
{
    closeSockets();
    QTcpServer::close();

    alog::Line logLine = log_verbose_m << "Stop listener";
    if (!name().isEmpty())
        logLine << log_format(" '?'", name());

    logLine << ". Connection point: " << _listenPoint;
}

void Listener::removeClosedSockets()
{
    removeClosedSocketsInternal();
}

void Listener::incomingConnection(SocketDescriptor socketDescriptor)
{
    Socket::Ptr socket {new Socket};
    incomingConnectionInternal(socket, socketDescriptor);
}

void Listener::connectSignals(base::Socket* socket)
{
    chk_connect_d(socket, SIGNAL(message(communication::Message::Ptr)),
                  this, SIGNAL(message(communication::Message::Ptr)))

    chk_connect_d(socket, SIGNAL(connected(communication::SocketDescriptor)),
                  this, SIGNAL(socketConnected(communication::SocketDescriptor)))

    chk_connect_d(socket, SIGNAL(disconnected(communication::SocketDescriptor)),
                  this, SIGNAL(socketDisconnected(communication::SocketDescriptor)))
}

void Listener::disconnectSignals(base::Socket* socket)
{
    QObject::disconnect(socket, 0, this, 0);
}

Listener& listener()
{
    return ::safe_singleton<Listener>();
}

} // namespace tcp
} // namespace transport
} // namespace communication

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
