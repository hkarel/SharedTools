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

  Модуль содержит базовые компоненты транспортной системы для TCP и UNIX
  сокетов.
*****************************************************************************/

#pragma once

#include "list.h"
#include "defmac.h"
#include "clife_base.h"
#include "clife_ptr.h"
#include "qt/thread/qthreadex.h"
#include "qt/communication/commands_base.h"
#include "qt/communication/serialize/functions.h"

#include <QtCore>
#include <atomic>

namespace communication {
namespace transport {
namespace base {

/**
  Определяет некоторые свойства для механизма коммуникаций
*/
class Properties
{
public:
    // Определяет уровень сжания потока.  Сжатие  выполняется  перед  отправкой
    // потока данных в TCP/Local сокет. Сжатие потока  выполняется  с использо-
    // ванием zip-алгоритма. Допускаются значения от 0 до 9,  что соответствует
    // уровням сжатия для zip-алгоритма.  Так, значение 0 будет  означать,  что
    // поток не должен быть сжат, а значение 9 будет соответствовать максималь-
    // ной степени сжатия. Значение -1 соответствует уровню сжатия по умолчанию
    int compressionLevel() const {return _compressionLevel;}
    void setCompressionLevel(int val);

    // Определяет размер потока данных (в байтах)  по достижении которого выпол-
    // няется сжатие потока перед отправкой в TCP/Local сокет. Значение по умол-
    // чанию равно 1024 байт
    int compressionSize() const {return _compressionSize;}
    void setCompressionSize(int val) {_compressionSize = val;}

    // Определяет нужно ли проверять совместимость версий протокола после
    // создания соединения
    bool checkProtocolCompatibility() const {return _checkProtocolCompatibility;}
    void setCheckProtocolCompatibility(bool val) {_checkProtocolCompatibility = val;}

    // Наименование сокета или листенера, используется для вывода лог-сообщений
    QString name() const {return _name;}
    void setName(const QString& val) {_name = val;}

protected:
    int _compressionLevel = {-1};
    int _compressionSize  = {1024};
    bool _checkProtocolCompatibility = {true};
    QString _name;
};

/**
  Класс содержит общие функции и поля для всех сокетов
*/
class SocketCommon : public QThreadEx
{
public:
    // Функции отправки сообщений
    bool send(const Message::Ptr&);

    // Удаляет из очереди сообщений на отправку сообщения с заданным
    // идентификатором команды
    void remove(const QUuidEx& command);

    // Возвращает количество сообщений в очереди команд на отправку в сокет.
    // Используется для оценки загруженности очереди
    int messagesCount() const;

    // Определяет нужно ли проверять, что входящая команда является неизвестной
    bool checkUnknownCommands() const {return _checkUnknownCommands;}
    void setCheckUnknownCommands(bool val) {_checkUnknownCommands = val;}

protected:
    Message::List _messagesHigh;
    Message::List _messagesNorm;
    Message::List _messagesLow;
    mutable QMutex _messagesLock;
    mutable QWaitCondition _messagesCond;

    // Счетчик для сообщений с нормальным приоритетом
    int _messagesNormCounter = {0};

    // Список команд неизвестных на принимающей стороне, позволяет передавать
    // только известные принимающей стороне команды
    QSet<QUuidEx> _unknownCommands;
    mutable std::atomic_flag _unknownCommandsLock = ATOMIC_FLAG_INIT;
    bool _checkUnknownCommands = {true};
};

/**
  Базовый класс для создания соединения и отправки сообщений. Используется как
  на клиентской, так и на серверной стороне
*/
class Socket : public SocketCommon,
               public clife_base,
               public Properties
{
    struct Allocator {void destroy(Socket* x) {if (x) x->release();}};

public:
    typedef clife_ptr<Socket> Ptr;
    typedef lst::List<Socket, lst::CompareItemDummy, Allocator> List;

    // Статус совместимости версий бинарного протокола
    enum class ProtocolCompatible {Yes, No, Undefined};

    // Возвращает статус подключения с учетом, того что было выполнено
    // подключение к сокету и была выполнена проверка на совместимость
    // версий протокола
    bool isConnected() const;

    // Возвращает TRUE в случае существования работоспособного сокет-соединения
    bool socketIsConnected() const;

    // Возвращает TRUE для UNIX сокета или для TCP сокета, когда он работает
    // по localhost
    bool isLocal() const;

    // Возвращает статус проверки совместимости версий протокола
    ProtocolCompatible protocolCompatible() const;

    // Возвращает тип сокета
    SocketType type() const {return _type;}

    // Числовой идентификатор сокета
    SocketDescriptor socketDescriptor() const;

    // Выполняет подключение к удаленному сокету с параметрами определенными
    // в методе init() в классе-наследнике
    void connect();

    // Разрывает соединение с удаленным сокетом
    void disconnect(unsigned long time = ULONG_MAX);

    // Ожидает (в секундах) подключения к удаленному хосту
    void waitConnection(int time = 0);

    // Возвращает формат сериализации сообщения. Формат сериализации возможно
    // задать только для клиентского сокета. На стороне сервера формат сериа-
    // лизации задается автоматически в зависимости от формата подключившегося
    // клиентского сокета. Формат сериализации должен быть задан до момента
    // установки TCP/Local соединения
    SerializationFormat messageFormat() const {return _messageFormat;}
    void setMessageFormat(SerializationFormat val);

signals:
    // Сигнал эмитируется при получении сообщения
    void message(const communication::Message::Ptr&);

    // Сигнал эмитируется после выполнения двух условий:
    // 1) Установлено соединение с TCP/Local сокетом;
    // 2) Проверка совместимости версий протокола выполнена успешно
    void connected(communication::SocketDescriptor);

    // Сигнал эмитируется после разрыва TCP/Local соединения
    void disconnected(communication::SocketDescriptor);

private slots:
    // Обработчик сигнала QTcpSocket::disconnected()
    void socketDisconnected();

protected:
    Socket(SocketType type);

    void run() override;
    void emitMessage(const communication::Message::Ptr&);

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

    virtual void messageInit(Message::Ptr&) = 0;
    virtual void fillUnknownMessage(const Message::Ptr&, data::Unknown&) = 0;

    // Признак того, что сокет был создан  на стороне listener-а, используется
    // для определения порядка обмена сигнатурами протоколов
    bool isListenerSide() const {return _isListenerSide;}
    void setListenerSide(bool val) {_isListenerSide = val;}

    // Возвращает TRUE когда сокет уже помещен в список listener-а, используется
    // для предотвращения преждевременного эмитирования сигнала connected()
    bool isInsideListener() const {return _isInsideListener;}
    void setInsideListener(bool val) {_isInsideListener = val;}

    // Вспомогательная функция, используется для связывания сокета созданного
    // в Listener
    SocketDescriptor initSocketDescriptor() const {return _initSocketDescriptor;}
    void setInitSocketDescriptor(SocketDescriptor val) {_initSocketDescriptor = val;}

private:
    Q_OBJECT
    DISABLE_DEFAULT_FUNC(Socket)

    const SocketType _type;
    volatile ProtocolCompatible _protocolCompatible = {ProtocolCompatible::Undefined};

    typedef QPair<SerializationFormat, QUuidEx /*сигнатура формата*/>  ProtocolSign;
    QVector<ProtocolSign> _protocolMap;

    // Формат сериализации сообщения (не контента)
    SerializationFormat _messageFormat = {SerializationFormat::BProto};

    bool _serializationSignatureRead = {false};
    bool _serializationSignatureWrite = {false};

    bool _isListenerSide = {false};
    volatile bool _isInsideListener = {false};

    SocketDescriptor _initSocketDescriptor = {-1};
    mutable std::atomic_flag _socketLock = ATOMIC_FLAG_INIT;

    friend class Listener;
};

/**
  Базовый класс для получения запросов на соединения от клиентских частей
  с последующей установкой соединения с ними, так же используется для приема
  и отправки сообщений
*/
class Listener : public Properties
{
public:
    // Возвращает список подключенных сокетов
    Socket::List sockets() const;

    // Функция отправки сообщений.
    // Параметр excludeSockets используется когда отправляемое сообщение имеет
    // тип Event. На сокеты содержащиеся в excludeSockets сообщение отправлено
    // не будет
    void send(const Message::Ptr& message,
              const SocketDescriptorSet& excludeSockets = SocketDescriptorSet()) const;

    void send(const Message::Ptr& message, SocketDescriptor excludeSocket) const;

    // Возвращает сокет по его идентификатору
    Socket::Ptr socketByDescriptor(SocketDescriptor) const;

    // Добавляет сокет в коллекцию сокетов
    void addSocket(const Socket::Ptr&);

    // Извлекает сокет из коллекции сокетов
    Socket::Ptr releaseSocket(SocketDescriptor);

    // Определяет нужно ли проверять, что входящая команда является неизвестной
    bool checkUnknownCommands() const {return _checkUnknownCommands;}
    void setCheckUnknownCommands(bool val) {_checkUnknownCommands = val;}

protected:
    Listener() = default;
    void closeSockets();
    void removeClosedSocketsInternal();
    void incomingConnectionInternal(Socket::Ptr, SocketDescriptor);

    virtual void connectSignals(Socket*) = 0;
    virtual void disconnectSignals(Socket*) = 0;

    // Используется для удаления сокетов для которых остановлен поток обработки
    QTimer _removeClosedSockets;

private:
    DISABLE_DEFAULT_COPY(Listener)

    Socket::List _sockets;
    mutable QMutex _socketsLock;
    bool _checkUnknownCommands = {true};
};

} // namespace base

// Функция отправки сообщений.
// Параметр excludeSockets используется когда отправляемое сообщение имеет
// тип Event. На сокеты содержащиеся в excludeSockets сообщение отправлено
// не будет
void send(const base::Socket::List& sockets,
          const Message::Ptr& message,
          const SocketDescriptorSet& excludeSockets = SocketDescriptorSet());

void send(const base::Socket::List& sockets,
          const Message::Ptr& message,
          SocketDescriptor excludeSocket);

// Вспомогательная функция
base::Socket::List concatSockets(const base::Listener& listener);

// Возвращает единый список сокетов для заданных listener-ов
template<typename... Args>
base::Socket::List concatSockets(const base::Listener& listener, const Args&... args)
{
    base::Socket::List sl = concatSockets(args...);
    base::Socket::List ss = listener.sockets();
    for (int i = 0; i < ss.count(); ++i)
        sl.add(ss.release(i, lst::CompressList::No));

    return sl;
}

} // namespace transport
} // namespace communication
