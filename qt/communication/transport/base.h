/*****************************************************************************
  Модуль содержит базовые компоненты транспортной системы для TCP и UNIX
  сокетов.

*****************************************************************************/

#pragma once

#include "defmac.h"
#include "container_ptr.h"
#include "simple_ptr.h"
#include "qt/thread/qthreadex.h"
#include "qt/communication/commands_base.h"
#include "qt/communication/message.h"
#include "qt/communication/functions.h"

#include <QtCore>
#include <atomic>

namespace communication {
namespace transport {
namespace base {

template<typename> class Listener;

/**
  Определяет некоторые свойства для механизма коммуникаций
*/
class Properties
{
public:
    // Определяет уровень сжания потока. Сжатие выполняется перед отправкой
    // потока данных в TCP-сокет. Сжатие потока выполняется с использованием
    // zip-алгоритма. Допускаются значения от 0 до 9, что соответствует уровням
    // сжатия для zip-алгоритма. Так, значение 0 будет означать, что поток
    // не должен быть сжат, а значение 9 будет соответствовать максимальной
    // степени сжатия.
    // Значение -1 соответствует уровню сжатия по умолчанию.
    int compressionLevel() const {return _compressionLevel;}
    void setCompressionLevel(int val);

    // Определяет размер потока данных (в байтах) по достижении которого выпол-
    // няется сжатие потока перед отправкой в TCP-сокет. Значение по умолчанию
    // 1024 байт.
    int compressionSize() const {return _compressionSize;}
    void setCompressionSize(int val) {_compressionSize = val;}

    // Определяет нужно ли проверять совместимость бинарных протоколов сразу
    // после установления TCP-соединения
    bool checkProtocolCompatibility() const {return _checkProtocolCompatibility;}
    void setCheckProtocolCompatibility(bool val) {_checkProtocolCompatibility = val;}

    // Определяет нужно ли проверять, что входящая команда является неизвестной
    bool checkUnknownCommands() const {return _checkUnknownCommands;}
    void setCheckUnknownCommands(bool val) {_checkUnknownCommands = val;}

protected:
    int _compressionLevel = {-1};
    int _compressionSize  = {1024};
    bool _checkProtocolCompatibility = true;
    bool _checkUnknownCommands = true;
};


/**
  Базовый класс для создания соединения и отправки сообщений. Используется как
  на клиентской, так и на серверной стороне.
*/
class Socket : public QThreadEx, public Properties
{
public:
    typedef container_ptr<Socket> Ptr;

    // Статус совместимости версий бинарного протокола
    enum class BinaryProtocol {Compatible, Incompatible, Undefined};

    // Возвращает статус подключения с учетом, того что было выполнено
    // подключение к сокету и была выполнена проверка на совместимость
    // версий бинарного протокола
    bool isConnected() const;

    // Возвращает TRUE в случае существования работоспособного сокет-соединения
    bool socketIsConnected() const;

    // Возвращает TRUE для UNIX сокета или для TCP сокета, когда он работает
    // по localhost.
    bool isLocal() const;

    // Возвращает статус проверки совместимости версий бинарного протокола
    BinaryProtocol binaryProtocolStatus() const;

    // Числовой идентификатор сокета
    virtual SocketDescriptor socketDescriptor() const;

    // Выполняет подключение к удаленному сокету с параметрами определенными
    // в методе init() в классе-наследнике
    void connect();

    // Разрывает соединение с удаленным сокетом
    void disconnect(unsigned long time = ULONG_MAX);

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

    // Возвращает количество сообщений в очереди команд на отправку в TCP-сокет.
    // Используется для оценки загруженности очереди.
    int messagesCount() const {return _messagesCount;}

    // Ожидает (в секундах) подключения к удаленному хосту.
    void waitConnection(int time = 0);

    // Используется для связывания сокета созданного в Listener.
    SocketDescriptor initSocketDescriptor() const {return _initSocketDescriptor;}
    void setInitSocketDescriptor(SocketDescriptor val) {_initSocketDescriptor = val;}

signals:
    // Сигнал эмитируется при получении сообщения
    void message(communication::Message::Ptr);

    // Сигнал эмитируется после установки TCP-соединения и после
    // проверки совместимости версий бинарного протокола.
    void connected(communication::SocketDescriptor);

    // Сигнал эмитируется после разрыва TCP-соединения
    void disconnected(communication::SocketDescriptor);

private slots:
    // Обработчик сигнала QTcpSocket::disconnected()
    void socketDisconnected();

protected:
    Socket();
    void run() override;

    virtual void socketCreate() = 0;
    virtual bool socketInit() = 0;

    virtual bool isLocalInternal() const = 0;
    virtual SocketDescriptor socketDescriptorInternal() const = 0;
    virtual bool socketIsConnectedInternal() const = 0;
    virtual void printSocketError(const char* file, const char* func, int line,
                                  const char* module) = 0;

    virtual qint64 socketBytesAvailable() const = 0;
    virtual qint64 socketBytesToWrite() const = 0;
    virtual qint64 socketRead(char* data, qint64 maxlen) = 0;
    virtual qint64 socketWrite(const char* data, qint64 len) = 0;
    virtual bool   socketWaitForReadyRead(int msecs) = 0;
    virtual bool   socketWaitForBytesWritten(int msecs) = 0;
    virtual void   socketClose() = 0;

    virtual Message::Ptr messageFromByteArray(const BByteArray&) = 0;
    virtual void fillUnknownMessage(const Message::Ptr&, data::Unknown&) = 0;

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Socket)

    // Список команд неизвестных на принимающей стороне, позволяет передавать
    // только известные принимающей стороне команды.
    QSet<QUuidEx> _unknownCommands;
    mutable std::atomic_flag _unknownCommandsLock = ATOMIC_FLAG_INIT;

    volatile BinaryProtocol _binaryProtocolStatus = {BinaryProtocol::Undefined};

    Message::List _messagesHigh;
    Message::List _messagesNorm;
    Message::List _messagesLow;
    mutable QMutex _messagesLock;

    volatile int _messagesCount = {0};
    int _messagesNormCounter = {0};

    static const QUuidEx _protocolSignature;
    bool _protocolSignatureRead = {false};
    bool _protocolSignatureWrite = {false};

    SocketDescriptor _initSocketDescriptor = {-1};
    mutable std::atomic_flag _socketLock = ATOMIC_FLAG_INIT;
};


/**
  Базовый класс для получения запросов на соединения от клиентских частей
  с последующей установкой соединения с ними, так же используется для приема
  и отправки сообщений.
*/
template<typename SocketT>
class Listener : public Properties
{
public:
    typedef typename SocketT::Ptr SocketPtr;
    typedef QVector<SocketPtr> SocketList;

    // Возвращает список подключенных сокетов
    SocketList sockets() const;

    // Функция отправки сообщений.
    // Параметр excludeSockets используется когда отправляемое сообщение имеет
    // тип Event. На сокеты содержащиеся в excludeSockets сообщение отправлено
    // не будет.
    void send(const Message::Ptr& message,
              const SocketDescriptorSet& excludeSockets = SocketDescriptorSet()) const;

    void send(const Message::Ptr& message, SocketDescriptor excludeSocket) const;

    // Возвращает сокет по его идентификатору.
    // Потенциальная уязвимость: если эта функция будет вызваться из обработ-
    // чика сигнала Socket::connected(), то есть вероятность, что на момент
    // вызова искомый сокет еще не будет добавлен с список Listener::_sockets,
    // следовательно  функция  вернет отрицательный результат,  хотя на самом
    // деле сокет уже существует и работает. На практике вероятность подобного
    // исхода событий достаточно низкая, т.к. до момента испускания сигнала
    // Socket::connected() выполняется ряд операций, и за это время новый
    // сокет уже будет добавлен с список Listener::_sockets.
    SocketPtr socketByDescriptor(SocketDescriptor) const;

    // Добавляет сокет в коллекцию сокетов
    void addSocket(const SocketPtr&);

    // Извлекает сокет из коллекции сокетов
    SocketPtr releaseSocket(SocketDescriptor);

protected:
    Listener() = default;
    void closeSockets();
    void removeClosedSocketsInternal();
    void incomingConnectionInternal(SocketDescriptor);

    virtual void connectSignals(base::Socket*) = 0;
    virtual void disconnectSignals(base::Socket*) = 0;

    // Используется для удаления сокетов для которых остановлен поток обработки
    QTimer _removeClosedSockets;

private:
    DISABLE_DEFAULT_COPY(Listener)

    SocketList _sockets;
    mutable QMutex _socketsLock;
};

} // namespace base
} // namespace transport
} // namespace communication

#include "base_impl.h"
