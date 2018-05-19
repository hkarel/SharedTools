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
  ---

  В модуле реализованы механизмы доставки сообщений между программными
  компонентами с использование UDP протокола.
*****************************************************************************/

#pragma once

#include "defmac.h"
#include "container_ptr.h"
#include "simple_ptr.h"
#include "qt/thread/qthreadex.h"
#include "qt/communication/message.h"
#include "qt/communication/functions.h"
#include "qt/communication/host_point.h"
#include "qt/communication/transport/base.h"

#include <QtCore>
#include <QUdpSocket>
#include <QHostAddress>
#include <atomic>

namespace communication {
namespace transport {
namespace udp {

/**
  Класс для отправки сообщений с использованием UDP-протокола.
*/
class Socket : public base::SocketCommon
{
public:
    typedef container_ptr<Socket> Ptr;

    Socket();
    bool init(const HostPoint& bindPoint,
              QUdpSocket::BindMode bindMode = QUdpSocket::DefaultForPlatform);

    // Ожидает (в секундах) выполнение привязки UDP-сокета к адресу и порту
    // определенными в методе init().
    void waitBinding(int time = 0);

    // Адрес и порт с которыми связан UDP-сокет
    HostPoint bindPoint() const {return _bindPoint;}

    // Возвращает TRUE если UDP-сокет был связан с параметрами address и port,
    // определенными в методе init().
    bool isBound() const;

    // Числовой идентификатор сокета
    SocketDescriptor socketDescriptor() const;

    // Сообщения приходящие с указанных в этом списке адресов будут отброшены.
    // На данный момент предполагается использование этого списка для предотвра-
    // щения получения собственных сообщений отправленных с помощью широковеща-
    // тельной рассылки.
    QList<QHostAddress> discardAddresses() const;
    void setDiscardAddresses(const QList<QHostAddress>&);

signals:
    // Сигнал эмитируется при получении сообщения
    void message(const communication::Message::Ptr&);

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Socket)

    void run() override;

private:
    simple_ptr<QUdpSocket> _socket;
    mutable std::atomic_flag _socketLock = ATOMIC_FLAG_INIT;

    HostPoint _bindPoint;
    QUdpSocket::BindMode _bindMode;
    QList<QHostAddress> _discardAddresses;
    mutable std::atomic_flag _discardAddressesLock = ATOMIC_FLAG_INIT;

    template<typename T> friend T* allocator_ptr<T>::create();
};

Socket& socket();

} // namespace udp
} // namespace transport
} // namespace communication
