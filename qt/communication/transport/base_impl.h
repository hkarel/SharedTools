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

#include "qt/communication/logger_operators.h"

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Transport")

namespace communication {
namespace transport {
namespace base {

//------------------------------- Listener -----------------------------------

template<typename SocketT>
void Listener<SocketT>::closeSockets()
{
    _removeClosedSockets.stop();
    SocketList sockets = this->sockets();
    for (Socket::Ptr s : sockets)
        if (s->isRunning())
        {
            s->stop();
            s->wait();
        }
    //QTcpServer::close();
    //log_verbose_m << "Stop listener of connection with params: " << _listenPoint;
}

template<typename SocketT>
typename Listener<SocketT>::SocketList Listener<SocketT>::sockets() const
{
    SocketList sockets;
    QMutexLocker locker(&_socketsLock); (void) locker;
    for (const SocketPtr& s : _sockets)
        if (s->isRunning())
            sockets.append(s);
    return sockets;
}

template<typename SocketT>
void Listener<SocketT>::send(const Message::Ptr& message,
                            const SocketDescriptorSet& excludeSockets) const
{
    SocketList sockets = this->sockets();
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

template<typename SocketT>
void Listener<SocketT>::send(const Message::Ptr& message,
                            SocketDescriptor excludeSocket) const
{
    SocketDescriptorSet excludeSockets;
    excludeSockets << excludeSocket;
    send(message, excludeSockets);
}

template<typename SocketT>
typename Listener<SocketT>::SocketPtr Listener<SocketT>::socketByDescriptor(SocketDescriptor descr) const
{
    QMutexLocker locker(&_socketsLock); (void) locker;
    for (const SocketPtr& s : _sockets)
        if (s->socketDescriptor() == descr)
            return s;

    return SocketPtr();
}

template<typename SocketT>
void Listener<SocketT>::addSocket(const SocketPtr& socket)
{
    if (socket.empty()
        || socket->socketDescriptor() == SocketDescriptor(-1))
        return;

    QMutexLocker locker(&_socketsLock); (void) locker;
    bool socketExists = false;
    for (int i = 0; i < _sockets.count(); ++i)
        if (_sockets.at(i)->socketDescriptor() == socket->socketDescriptor())
        {
            socketExists = true;
            break;
        }
    if (!socketExists)
    {
        _sockets.append(socket);
        connectSignals(socket.get());
    }
}

template<typename SocketT>
typename Listener<SocketT>::SocketPtr Listener<SocketT>::releaseSocket(SocketDescriptor descr)
{
    QMutexLocker locker(&_socketsLock); (void) locker;
    for (int i = 0; i < _sockets.count(); ++i)
        if (_sockets.at(i)->socketDescriptor() == descr)
        {
            SocketPtr socket = _sockets.at(i);
            disconnectSignals(socket.get());
            _sockets.remove(i);
            return socket;
        }
    return Socket::Ptr();
}

template<typename SocketT>
void Listener<SocketT>::removeClosedSocketsInternal()
{
    QMutexLocker locker(&_socketsLock); (void) locker;
    for (int i = 0; i < _sockets.size(); ++i)
        if (!_sockets.at(i)->isRunning())
        {
            _sockets.remove(i--);
            break;
        }
}

template<typename SocketT>
void Listener<SocketT>::incomingConnectionInternal(SocketDescriptor socketDescriptor)
{
    SocketPtr socket = SocketPtr::create_ptr();
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
    _sockets.append(socket);
}


} // namespace base
} // namespace transport
} // namespace communication

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
