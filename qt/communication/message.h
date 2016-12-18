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
#include "command_type.h"
#include "communication_bserialize.h"

#include <QtCore>
#include <utility>

namespace communication {

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

public:
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

    // Очищает содержимое поля _conten
    void clearContent() {_content.clear();}

    // Возвращает TRUE если сообщение не содержит дополнительных данных
    bool contentIsEmpty() const {return _content.isEmpty();}

    // Тип пересылаемой команды.
    command::Type commandType() const;
    void setCommandType(command::Type);

    // Статус выполнения/обработки команды. См. описание command::ExecStatus
    command::ExecStatus commandExecStatus() const;
    void setCommandExecStatus(command::ExecStatus);

    // Приоритет сообщения
    Priority priority() const;
    void setPriority(Priority);

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

    // Признак, что контент сообщения находится в сжатом состоянии.
    Compression compression() const;

    // Функция выполняет сжатие контента сообщения. Параметр level определяет
    // уровень сжатия контента. Допускаются значения в диапазоне от 0 до 9,
    // что соответствует уровням сжатия для zip-алгоритма.
    // Если значение level равно -1, то уровень сжатия будет дефолтным
    // для используемого алгоритма.
    void compress(int level = -1, Compression compression = Compression::Zip);

    // Выполняет декомпрессию контента сообщения.
    void decompress();

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

    void decompress(BByteArray&) const;

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
        quint32 _flags; // Содержит значения всех флагов, используется
                        // при сериализации.
        struct {
            // Тип пересылаемого сообщения, значения в этом поле
            // соответствуют command::Type.
            // Резервируем 4 бита для возможного будущего расширения command::Type.
            quint32 _commandType: 4;

            // Статус выполнения/обработки команды, значения в этом поле
            // соответствуют command::ExecStatus.
            quint32 _commandExecStatus: 4;

            // Приоритет сообщения
            quint32 _priority: 2;

            // Признак, что контент сообщения находится в сжатом состоянии,
            // так же содержит информацию по алгоритму сжатия.
            quint32 _compression: 3;

            quint32 _reserved: 19;
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
