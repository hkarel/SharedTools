/*****************************************************************************
  В модуле реализованы механизмы доставки сообщений между программными компо-
  нентами. Клиентская и серверная части выполнены в одном модуле исключительно
  по соображениям удобства кодирования и дальнейшего сопровождения кода.

*****************************************************************************/

#pragma once

#include "defmac.h"
#include "container_ptr.h"
#include "qt/thread/qthreadex.h"
#include "qt/communication/message.h"
#include "qt/communication/functions.h"
#include "qt/communication/host_point.h"

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
class Socket : public QThreadEx
{
public:
    typedef container_ptr<Socket> Ptr;

    Socket();
    bool init(const HostPoint&);

    // Числовой идентификатор сокета
    SocketDescriptor socketDescriptor() const;

    // Адрес и порт с которыми связан UDP-сокет
    HostPoint bindPoint() const {return _bindPoint;}

    // Функции отправки сообщений.
    bool send(const Message::Ptr&);
    bool send(const QUuidEx& command);

    template<typename CommandDataT>
    bool send(const CommandDataT& data, Message::Type type = Message::Type::Command)
    {
        Message::Ptr message = createMessage(data, type);
        return send(message);
    }

    // Удаляет из очереди сообщений на отправку сообщения с заданным
    // идентификатором команды
    void remove(const QUuidEx& command);

    // Возвращает количество сообщений в очереди команд на отправку в UDP-сокет.
    // Используется для оценки загруженности очереди.
    int messagesCount() const {return _messagesCount;}

    // Определяет нужно ли проверять совместимость бинарных протоколов сразу
    // после установления UDP-соединения
    bool checkProtocolCompatibility() const {return _checkProtocolCompatibility;}
    void setCheckProtocolCompatibility(bool val) {_checkProtocolCompatibility = val;}

    // Определяет нужно ли проверять, что входящая команда является неизвестной
    bool checkUnknownCommands() const {return _checkUnknownCommands;}
    void setCheckUnknownCommands(bool val) {_checkUnknownCommands = val;}

    // Сообщения приходящие с указанных в этом списке адресов будут отброшены.
    // На данный момент предполагается использование этого списка для предотвра-
    // щения получения собственных сообщений отправленных с помощью широковеща-
    // тельной рассылки.
    QList<QHostAddress> discardAddresses() const;
    void setDiscardAddresses(const QList<QHostAddress>&);

signals:
    // Сигнал эмитируется при получении сообщения
    void message(communication::Message::Ptr);

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Socket)

    void run() override;
    //void setSocketDescriptor(SocketDescriptor);

private:
    // Список команд неизвестных на принимающей стороне, позволяет передавать
    // только известные принимающей стороне команды.
    QSet<QUuidEx> _unknownCommands;
    mutable std::atomic_flag _unknownCommandsLock = ATOMIC_FLAG_INIT;

    QUdpSocket _socket;
    HostPoint _bindPoint;

    Message::List _messagesHigh;
    Message::List _messagesNorm;
    Message::List _messagesLow;
    mutable QMutex _messagesLock;

    volatile int _messagesCount = {0};
    int _messagesNormCounter = {0};

    bool _checkProtocolCompatibility = true;
    bool _checkUnknownCommands = true;

    QList<QHostAddress> _discardAddresses;
    mutable std::atomic_flag _discardAddressesLock = ATOMIC_FLAG_INIT;

    template<typename T> friend T* allocator_ptr<T>::create();
};

Socket& socket();

} // namespace udp
} // namespace transport
} // namespace communication
