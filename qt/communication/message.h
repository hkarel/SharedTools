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
#include "communication_bserialize.h"

#include <QtCore>
#include <utility>

namespace communication {

#if QT_VERSION >= 0x050000
typedef qintptr SocketDescriptor;
#else
typedef int SocketDescriptor;
#endif


class Message : public clife_base
{
    struct Allocator
    {
        void destroy(Message* x) {if (x)  x->release();}
    };

public:
    typedef clife_ptr<Message> Ptr;
    typedef lst::List<Message, lst::CompareItemDummy, Allocator> List;

    // Тип пересылаемого сообщения
    enum Type
    {
        Request  = 0, // Сообщение содержит данные запроса соответствующие
                      // передаваемой команде.
        Responce = 1, // Сообщение содержит данные ответа соответствующие
                      // передаваемой команде.
        Event    = 2  // Данный тип сообщения похож на Request, но не пред-
                      // полагает получения ответа. Он используется для
                      // рассылки широковещательных сообщений о событиях.
    };

    // Статус выполнения/обработки команды. Используется для сообщений
    // с типом Responce.
    enum ExecStatus
    {
        Unknown = 0,
        Success = 1, // Сообщение было обработано успешно и содержит корректные
                     // ответные данные.
        Failed  = 2, // Сообщение не было обработано успешно, но результат
                     // не является ошибкой.
                     // В данном случае поле _content содержит данные в формате
                     // communication::data::MessageFailed.
        Error   = 3  // При обработке запроса произошла ошибка, и в качестве
                     // ответа отправляется сообщения с описанием причины ошибки.
                     // В данном случае поле _content содержит данные в формате
                     // communication::data::MessageError.
    };

public:
    // Персональный идентификатор сообщения.
    QUuidEx id() const {return _id;}

    // Идентификатор комманды
    QUuidEx command() const {return _command;}

    // Очищает содержимое поля _conten
    void clearContent() {_content.clear();}

    // Возвращает TRUE если сообщение не содержит дополнительных данных
    bool contentIsEmpty() const {return _content.isEmpty();}

    // Тип пересылаемого сообщения. См. описание enum Type
    Type type() const {return Type(_type);}
    void setType(Type val);

    // Статус выполнения/обработки команды. См. описание enum ExecStatus
    ExecStatus execStatus() const {return ExecStatus(_execStatus);}
    void setExecStatus(ExecStatus val);

    // Максимальное время жизни сообщения. Задается в секундах в формате UTC
    // от начала эпохи. Параметр представляет абсолютное значение времени по
    // достижении которого сообщение перестает быть актуальным.
    // Параметр задает тайм-аут для ожидания синхронных сообщений. Так тайм-аут
    // в 2 мин. можно задать следующим образом:
    //   setMaxTimeLife(std::time() + 2*60)
    quint64 maxTimeLife() const {return _maxTimeLife;}
    void setMaxTimeLife(quint64 val) {_maxTimeLife = val;}

    // Вспомогательный параметр, используется на стороне сервера для идентифика-
    // ции сокета с которого было получено сообщение.
    SocketDescriptor socketDescriptor() const {return _socketDescriptor;}
    void setSocketDescriptor(SocketDescriptor val) {_socketDescriptor = val;}

    // Вспомогательный параметр, используется для того чтобы сообщить функциям-
    // обработчикам сообщений о том, что сообщение уже было обработано ранее.
    // Таким образом последующие обработчики могут проигнорировать это сообщение.
    bool processed() const {return _processed;}
    void setProcessed(bool val) {_processed = val;}

    // Создает сообщение
    static Ptr create(const QUuidEx& command);

    // Функция записи данных
    template<typename... Args>
    bool writeContent(const Args&... args);

    // Функция чтения данных
    template<typename... Args>
    bool readContent(Args&... args) const;

    // Вспомогательные функции, используются для формирования сырого потока
    // данных для отправки в tcp-сокет.
    BByteArray toByteArray() const;
    static Ptr fromByteArray(const BByteArray&);

private:
    Message();
    Message(const QUuidEx& command);
    DISABLE_DEFAULT_COPY(Message)

    template<typename T, typename... Args>
    void writeInternal(QDataStream& s, const T& t, const Args&... args);
    void writeInternal(QDataStream&) {return;}

    template<typename T, typename... Args>
    void readInternal(QDataStream& s, T& t, Args&... args) const;
    void readInternal(QDataStream&) const {return;}

    // Функции сериализации данных
    DECLARE_B_SERIALIZE_FUNC

private:
    // Битовые флаги
    union {
        quint32 _flags;            // Содержит значения всех флагов, используется
                                   // при сериализации.
        struct {
            quint32 _type: 4;       // Тип пересылаемого сообщения, значения
                                    // в этом поле соответствуют enum Type.
                                    // Резервируем 4 бита для возможного будущего
                                    // расширения enum Type.
            quint32 _execStatus: 4; // Статус выполнения/обработки команды, значения
                                    // в этом поле соответствуют enum ExecStatus.
            quint32 _reserved: 24;
        };
    };

    // Зарезервировано для будущего использования
    quint32 _flags2;

    QUuidEx _id;
    QUuidEx _command;
    quint64 _maxTimeLife = {quint64(-1)};
    BByteArray _content;

    SocketDescriptor _socketDescriptor = {-1};
    bool _processed = {false};

    //template <typename... Args> friend Ptr::self_t Ptr::create_join_ptr(Args&&...);
    //template<typename DataT> friend typename Message::Ptr createMessage(const DataT&);
};


//------------------------- Implementation Message ---------------------------

template<typename... Args>
bool Message::writeContent(const Args&... args) {
    _content.clear();
    QDataStream s(&_content, QIODevice::WriteOnly);
    writeInternal(s, args...);
    return (s.status() == QDataStream::Ok);
}

template<typename... Args>
bool Message::readContent(Args&... args) const {
    QDataStream s(_content);
    readInternal(s, args...);
    return (s.status() == QDataStream::Ok);
}

template<typename T, typename... Args>
void Message::writeInternal(QDataStream& s, const T& t, const Args&... args) {
    if (s.status() != QDataStream::Ok)
        return;
    s << t;
    writeInternal(s, args...);
}

template<typename T, typename... Args>
void Message::readInternal(QDataStream& s, T& t, Args&... args) const {
    if (s.status() != QDataStream::Ok)
        return;
    s >> t;
    readInternal(s, args...);
}

} // namespace communication
