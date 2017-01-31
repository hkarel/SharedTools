/*****************************************************************************
  В модуле реализованы механизмы доставки сообщений между программными компо-
  нентами. Клиентская и серверная части выполнены в одном модуле исключительно
  по соображениям удобства кодирования и дальнейшего сопровождения кода.

*****************************************************************************/

#pragma once

#include "defmac.h"
#include "simple_ptr.h"
#include "safe_singleton.h"
#include "qt/thread/qthreadex.h"
#include "qt/communication/message.h"
#include "qt/communication/functions.h"

#include <QtCore>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <atomic>


namespace communication {
namespace transport {
namespace tcp {

class Sender;
class Listener;

/**
  Определяет несколько одинаковых свойств, которые могут использоваться
  в разных классах-наследниках.
*/
class Base
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

    // Определяет, что сообщение будет эмитироваться в формате MessageRaw
    //bool emitMessageRaw() const {return _emitMessageRaw;}
    //void setEemitMessageRaw(bool val) {_emitMessageRaw = val;}

protected:
    int _compressionLevel = {-1};
    int _compressionSize  = {1024};
    bool _checkProtocolCompatibility = true;
    bool _checkUnknownCommands = true;
    //bool _emitMessageRaw = false;
};


/**
  Базовый класс для создания соединения и отправки сообщений. Используется как
  на клиентской, так и на серверной стороне.
*/
class Socket : public QThreadEx, public Base
{
public:
    typedef container_ptr<Socket> Ptr;

    // Статус совместимости версий бинарного протокола
    enum class BinaryProtocol {Compatible, Incompatible, Undefined};

    // Возвращает статус подключения с учетом, того что было выполнено
    // подключение к TCP-сокету и была выполнена проверка на совместимость
    // версий бинарного протокола
    bool isConnected() const;

    // Возвращает TRUE в случае существования работоспособного TCP-соединения
    bool socketIsConnected() const;

    // Возвращает TRUE когда TCP-соединения работает по localhost
    bool isLoopback() const;

    // Возвращает статус проверки совместимости версий бинарного протокола
    BinaryProtocol binaryProtocolStatus() const;

    // Числовой идентификатор сокета
    SocketDescriptor socketDescriptor() const;

    // Адрес с которым установлено соединение
    QHostAddress address() const {return _address;}

    // Порт с которым установлено соединение
    quint16 port() const {return _port;}

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

signals:
    // Сигнал эмитируется при получении сообщения
    void message(communication::Message::Ptr);

    // Сигнал эмитируется после установления TCP-соединения и после
    // проверки совместимости версий бинарного протокола.
    void connected(communication::SocketDescriptor);

    // Сигнал эмитируется после разрыва TCP-соединения
    void disconnected(communication::SocketDescriptor);

private slots:
    // Обработчик сигнала QTcpSocket::disconnected()
    void socketDisconnected();

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Socket)

    Socket() = default;
    void run() override;
    void setSocketDescriptor(SocketDescriptor);

private:
    // Список команд неизвестных на принимающей стороне, позволяет передавать
    // только известные принимающей стороне команды.
    QSet<QUuidEx> _unknownCommands;
    mutable std::atomic_flag _unknownCommandsLock = ATOMIC_FLAG_INIT;

    simple_ptr<QTcpSocket> _socket;
    SocketDescriptor _socketDescriptor = {-1};

    QHostAddress _address;
    quint16 _port = {0};

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

    friend class Sender;
    friend class Listener;
    template<typename T> friend T* allocator_ptr<T>::create();
    //template <typename... Args> friend Ptr::self_t Ptr::create_join_ptr(Args&&...);
};


/**
  Используется для создания соединения и отправки сообщений на клиентской
  стороне.
*/
class Sender : public Socket
{
public:
    Sender();
    ~Sender();

    // Определяет параметры подключения к удаленному хосту
    bool init(const QHostAddress& address, int port);

    // Выполняет подключение к удаленному хосту с параметрами address и port,
    // определенными в методе init(). Если подключиться не удалось - функция
    // ждет 10 секунд и повторяет попытку. Так будет продолжаться либо до уста-
    // новления соединения, либо до явного вызова функций stop()/disconnect().
    void connect();

    // Разрывает соединение с удаленным хостом.
    void disconnect();

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Sender)
    void run() override;
};


/**
  Используется для получения запросов на соединения от клиентских частей
  с последующей установкой соединения с ними, так же используется для приема
  и отправки сообщений.
*/
class Listener : public QTcpServer, public Base
{
public:
    Listener();

    // Переводит Listener в режим приема внешних подключений
    bool init(const QHostAddress& address, int port);

    // Listener останавливает прием внешних подключений. Помимо этого все
    // активные соединения будут закрыты.
    void close();

    // Возвращает список подключенных сокетов
    QVector<Socket::Ptr> sockets() const;

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
    Socket::Ptr socketByDescriptor(SocketDescriptor) const;

signals:
    // Сигнал эмитируется при получении сообщения
    void message(communication::Message::Ptr);

    // Сигнал эмитируется после установки socket-ом соединения
    void socketConnected(communication::SocketDescriptor);

    // Сигнал эмитируется после разрыва socket-ом соединения
    void socketDisconnected(communication::SocketDescriptor);

private slots:
    void removeClosedSockets();

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Listener)
    void incomingConnection (SocketDescriptor socketDescriptor) override;

private:
    QVector<Socket::Ptr> _sockets;
    mutable QMutex _socketsLock;

    // Используется для удаления сокетов для которых остановлен поток обработки
    QTimer _removeClosedSockets;

    friend class Socket;
    template<typename T, int> friend T& ::safe_singleton();
};

} // namespace tcp
} // namespace transport

transport::tcp::Listener& listener();

} // namespace communication
