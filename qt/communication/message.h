/*****************************************************************************
  В модуле представлена структура сообщения, которая используется для обмена
  сообщениями и данными между удаленными модулями системы.

*****************************************************************************/

#pragma once

#include "_list.h"
#include "defmac.h"
#include "clife_base.h"
#include "clife_ptr.h"
#include "qt/quuidex.h"
#include "qt/bserialize.h"
#include "qt/communication/host_point.h"

#include <QtCore>
#include <utility>

namespace communication {

namespace transport {
namespace tcp {class Socket;}
namespace udp {class Socket;}
}

#if QT_VERSION >= 0x050000
typedef qintptr SocketDescriptor;
#else
typedef int SocketDescriptor;
#endif
typedef QSet<SocketDescriptor> SocketDescriptorSet;


class Message : public clife_base
{
    struct Allocator
    {
        void destroy(Message* x) {if (x)  x->release();}
    };

public:
    typedef clife_ptr<Message> Ptr;
    typedef lst::List<Message, lst::CompareItemDummy, Allocator> List;

    enum class Type : quint32
    {
        Unknown = 0,
        Command = 1, // Сообщение-команда. Это может быть сообщение с командой
                     // на выполнение действия, либо это может быть запрос
                     // на получение данных. Предполагается, что в ответ
                     // на данное сообщение придет сообщение с типом Answer.
        Answer  = 2, // Ответ на сообщением с типом Command.
        Event   = 3  // Данный тип сообщения похож на Command, но не предполагает
                     // получения ответа (Answer). Он используется для рассылки
                     // широковещательных сообщений о событиях.
    };

    // Статус выполнения/обработки сообщения. Используется в сообщениях
    // с типом Answer для того чтобы уведомить другую сторону о статусе
    // выполнения команды с типом Command.
    enum class ExecStatus : quint32
    {
        Unknown = 0,
        Success = 1, // Сообщение было обработано успешно и содержит корректные
                     // ответные данные.
        Failed  = 2, // Сообщение не было обработано успешно, но результат
                     // не является ошибкой.
                     // В данном случае сообщение (Message) будет содержать
                     // данные в формате communication::data::MessageFailed.
        Error   = 3  // При обработке сообщения произошла ошибка, и в качестве
                     // ответа отправляется сообщения с описанием причины ошибки.
                     // В данном случае сообщение (Message) будет содержать данные
                     // в формате communication::data::MessageError.
    };

    enum class Priority : quint32
    {
        High   = 0,
        Normal = 1,
        Low    = 2
        // Reserved = 3
    };

    enum class Compression : quint32
    {
        None = 0,
        Zip  = 1,
        Lzma = 2,
        Ppmd = 3,
        Disable = 7 // Используется в тех случаях когда нужно явно запретить
                    // сжатие сообщения при его отправке в TCP сокет.
                    // Это может потребоваться когда контент изначально сжат,
                    // например, при отправке JPG, PNG, и прочих подобных
                    // форматов.
    };

    // Персональный идентификатор сообщения.
    QUuidEx id() const {return _id;}

    // Идентификатор комманды
    QUuidEx command() const {return _command;}

    // Функции возвращают нижнюю и верхнюю границы версий бинарного протокола
    quint16 protocolVersionLow() const {return _protocolVersionLow;}
    quint16 protocolVersionHigh() const {return _protocolVersionHigh;}

    // Удаляет контент сообщения
    void clearContent() {_content.clear();}

    // Возвращает TRUE если сообщение не содержит дополнительных данных
    bool contentIsEmpty() const {return _content.isEmpty();}

    // Тип пересылаемой команды.
    Type type() const;
    void setType(Type);

    // Статус выполнения/обработки команды. См. описание command::ExecStatus
    ExecStatus execStatus() const;
    void setExecStatus(ExecStatus);

    // Приоритет сообщения
    Priority priority() const;
    void setPriority(Priority);

    // Передает пользовательские данные без сохранения их в поле content.
    // Это позволяет сократить количество ресурсоемких операций сериализации/
    // десериализации данных необходимых для поля content.
    quint32 tag() const {return _tag;}
    void setTag(quint32 val) {_tag = val;}

    // Максимальное время жизни сообщения. Задается в секундах в формате UTC
    // от начала эпохи. Параметр представляет абсолютное значение времени по
    // достижении которого сообщение перестает быть актуальным.
    // Параметр задает тайм-аут для ожидания синхронных сообщений. Так тайм-аут
    // в 2 мин. можно задать следующим образом:
    //   setMaxTimeLife(std::time() + 2*60)
    quint64 maxTimeLife() const {return _maxTimeLife;}
    void setMaxTimeLife(quint64 val) {_maxTimeLife = val;}

    // Адрес и порт хоста с которого было получено сообщение
    const HostPoint& sourcePoint() const {return _sourcePoint;}

    // Адреса и порты хостов назначения. Параметр используется для отправки
    // сообщения через UDP-сокет. В случае если параметр destinationPoints
    // не содержит ни одного элемента HostPoint и при этом параметр sourcePoint
    // не пуст (!isNull), то в этом случае сообщение будет отправлено на UDP-
    // сокет с точкой назначения sourcePoint.
    HostPoint::Set& destinationPoints() {return _destinationPoints;}

    // Вспомогательный параметр, используется на стороне TCP-сервера для иден-
    // тификации TCP-сокета принявшего сообщение.
    SocketDescriptor socketDescriptor() const {return _socketDescriptor;}

    // Параметр содержит идентификаторы TCP-сокетов на которые нужно отправить
    // сообщение. В случае если параметр destinationSocketDescriptors не со-
    // держит ни одного идентификатора и при этом параметр socketDescriptor
    // не равен -1, то в этом случае сообщение будет отправлено на TCP-сокет
    // с идентификатором socketDescriptor.
    SocketDescriptorSet& destinationSocketDescriptors();

    // Вспомогательный параметр, используется для того чтобы сообщить функциям-
    // обработчикам сообщений о том, что сообщение уже было обработано ранее.
    // Таким образом последующие обработчики могут проигнорировать это сообщение.
    bool processed() const {return _processed;}
    void setProcessed(bool val) {_processed = val;}

    // Возвращает информацию о том, в каком состоянии (сжатом или несжатом)
    // находится контент сообщения.
    Compression compression() const;

    // Функция выполняет сжатие контента сообщения. Параметр level определяет
    // уровень сжатия контента. Допускаются значения в диапазоне от 0 до 9,
    // что соответствует уровням сжатия для zip-алгоритма.
    // Если значение level равно -1, то уровень сжатия будет дефолтным
    // для используемого алгоритма.
    void compress(int level = -1, Compression compression = Compression::Zip);

    // Запрещает сжатие сообщения на уровне сетевого сокета
    void disableCompress() {compress(-1, Compression::Disable);}

    // Выполняет декомпрессию контента сообщения.
    void decompress();

    // Создает сообщение
    static Ptr create(const QUuidEx& command);

    // Создает отдельную копию сообщения, которую можно использовать как сообще-
    // ние-ответ. Основная причина введения данной функции заключается в том,
    // что не всегда возможно изменять параметры исходного сообщения, особенно
    // в многопоточном приложении.
    // Параметры, которые не клонируются (инициализируются дефолтными
    // значениями):
    //   - контент сообщения;
    //   - compression (информация о сжатии контента);
    //   - destinationPoints;
    //   - destinationSocketDescriptors.
    // Параметры, которые инициализируются предопределенными значениями:
    //   - type = Answer;
    //   - execStatus = Success
    Ptr cloneForAnswer() const;

    // Функция записи данных
    template<typename... Args>
    bool writeContent(const Args&... args);

    // Функция чтения данных
    template<typename... Args>
    bool readContent(Args&... args) const;

    // Вспомогательные функции, используются для формирования сырого потока
    // данных для отправки в сетевой сокет.
    // Параметр udpSignature используется при передаче сообщения через UDP
    // сокет.
    BByteArray toByteArray() const;
    void toDataStream(QDataStream&) const;

    static Ptr fromByteArray(const BByteArray&);
    static Ptr fromDataStream(QDataStream&);

    // Возвращает максимально возможную длину сообщения в сериализованном виде.
    // Данный метод используется для оценки возможности передачи сообщения
    // посредством UDP датаграммы.
    int size() const;

private:
    Message();
    DISABLE_DEFAULT_COPY(Message)

    void decompress(BByteArray&) const;

    template<typename T, typename... Args>
    void writeInternal(QDataStream& s, const T& t, const Args&... args);
    void writeInternal(QDataStream&) {return;}

    template<typename T, typename... Args>
    void readInternal(QDataStream& s, T& t, Args&... args) const;
    void readInternal(QDataStream&) const {return;}

    void setSourcePoint(const HostPoint& val) {_sourcePoint = val;}
    void setSocketDescriptor(SocketDescriptor val) {_socketDescriptor = val;}

private:
    QUuidEx _id;
    QUuidEx _command;

    quint16 _protocolVersionLow  = {BPROTOCOL_VERSION_LOW};
    quint16 _protocolVersionHigh = {BPROTOCOL_VERSION_HIGH};

    // Битовые флаги
    union {
        quint32 _flags; // Поле содержит значения всех флагов, используется
                        // при сериализации
        struct {
            // Тип пересылаемого сообщения, соответствует enum Type
            quint32 _type: 3;

            // Статус выполнения команды, соответствует enum ExecStatus
            quint32 _execStatus: 3;

            // Приоритет сообщения, соответствует enum Priority
            quint32 _priority: 2;

            // Признак что контент сообщения находится в сжатом состоянии,
            // так же содержит информацию по алгоритму сжатия, соответствует
            // enum Compression
            quint32 _compression: 3;

            // Признаки 'EmptyTraits'. Признаки используются для оптимизации
            // размера сообщения при его сериализации.
            mutable quint32 _tagIsEmpty: 1;
            mutable quint32 _maxTimeLifeIsEmpty: 1;
            mutable quint32 _contentIsEmpty: 1;

            quint32 _reserved: 17;

            // Признак 'EmptyTraits'. Признак используется для оптимизации
            // размера сообщения при его сериализации. Данный признак идет
            // последним битом в поле _flags.
            mutable quint32 _flags2IsEmpty: 1;
        };
    };

    // Зарезервировано для будущего использования
    quint32 _flags2;

    quint32 _tag = {0};
    quint64 _maxTimeLife = {quint64(-1)};
    BByteArray _content;

    HostPoint _sourcePoint;
    HostPoint::Set _destinationPoints;
    SocketDescriptor _socketDescriptor = {-1};
    SocketDescriptorSet _destinationSocketDescriptors;
    volatile bool _processed = {false};

    friend class transport::tcp::Socket;
    friend class transport::udp::Socket;
};


//------------------------- Implementation Message ---------------------------

template<typename... Args>
bool Message::writeContent(const Args&... args)
{
    _content.clear();
    QDataStream s(&_content, QIODevice::WriteOnly);
    writeInternal(s, args...);
    return (s.status() == QDataStream::Ok);
}

template<typename... Args>
bool Message::readContent(Args&... args) const
{
    BByteArray content;
    decompress(content);
    QDataStream s(content);
    readInternal(s, args...);
    return (s.status() == QDataStream::Ok);
}

template<typename T, typename... Args>
void Message::writeInternal(QDataStream& s, const T& t, const Args&... args)
{
    if (s.status() != QDataStream::Ok)
        return;
    s << t;
    writeInternal(s, args...);
}

template<typename T, typename... Args>
void Message::readInternal(QDataStream& s, T& t, Args&... args) const
{
    if (s.status() != QDataStream::Ok)
        return;
    s >> t;
    readInternal(s, args...);
}

} // namespace communication
