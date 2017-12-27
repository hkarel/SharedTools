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
  компонентами с использование UNIX-сокетов.
*****************************************************************************/

#pragma once

#include "simple_ptr.h"
#include "safe_singleton.h"
#include "qt/communication/transport/base.h"

#include <QLocalSocket>
#include <QLocalServer>

namespace communication {
namespace transport {
namespace local {

/**
  Используется для создания соединения и отправки сообщений на клиентской
  стороне.
*/
class Socket : public base::Socket
{
public:
    typedef clife_ptr<Socket> Ptr;

    Socket() : base::Socket(SocketType::Local) {}

    // Определяет параметры подключения к удаленному хосту
    bool init(const QString& serverName);

    // Наименование сокета с которым установлено соединение
    QString serverName() const {return _serverName;}

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Socket)

    void socketCreate() override;
    bool socketInit() override;

    // Возвращает TRUE когда TCP-сокет работает по localhost.
    bool isLocalInternal() const override;
    SocketDescriptor socketDescriptorInternal() const override;
    bool socketIsConnectedInternal() const override;
    void printSocketError(const char* file, const char* func, int line,
                          const char* module) override;

    qint64 socketBytesAvailable() const override;
    qint64 socketBytesToWrite() const override;
    qint64 socketRead(char* data, qint64 maxlen) override;
    qint64 socketWrite(const char* data, qint64 len) override;
    bool   socketWaitForReadyRead(int msecs) override;
    bool   socketWaitForBytesWritten(int msecs) override;
    void   socketClose() override;

    Message::Ptr messageFromByteArray(const BByteArray&) override;
    void fillUnknownMessage(const Message::Ptr&, data::Unknown&) override;

private:
    simple_ptr<QLocalSocket> _socket;
    QString _serverName;

    // Используется для вывода в лог сообщений об уже закрытом сокете.
    SocketDescriptor _printSocketDescriptor = {-1};

    template<typename T> friend T* allocator_ptr<T>::create();
};


/**
  Используется для получения запросов на соединения от клиентских частей
  с последующей установкой соединения с ними, так же используется для приема
  и отправки сообщений.
*/
class Listener : public QLocalServer, public base::Listener
{
public:
    Listener();

    // Инициализация режима приема внешних подключений
    bool init(const QString& serverName);

    // Listener останавливает прием внешних подключений. Помимо этого все
    // активные соединения будут закрыты.
    void close();

signals:
    // Сигнал эмитируется при получении сообщения
    void message(communication::Message::Ptr);

    // Сигнал эмитируется после установки socket-ом соединения и после
    // проверки совместимости версий бинарного протокола.
    void socketConnected(communication::SocketDescriptor);

    // Сигнал эмитируется после разрыва socket-ом соединения
    void socketDisconnected(communication::SocketDescriptor);

private slots:
    void removeClosedSockets();

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Listener)
    void incomingConnection(quintptr) override;

    void connectSignals(base::Socket*) override;
    void disconnectSignals(base::Socket*) override;

    QString _serverName;

    template<typename T, int> friend T& ::safe_singleton();
};

Listener& listener();

} // namespace local
} // namespace transport
} // namespace communication

