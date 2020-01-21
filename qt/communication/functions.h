/*****************************************************************************
  The MIT License

  Copyright © 2015 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  В модуле собраны функции общего назначения для работы с коммуникационными
  механизмами.
*****************************************************************************/

#pragma once

#include "prog_abort.h"
#include "logger/logger.h"
#include "qt/logger/logger_operators.h"
#include "qt/communication/logger_operators.h"
#include "qt/communication/commands_base.h"
#include "qt/communication/error_sender.h"
#include "qt/communication/message.h"
#include "qt/communication/serialization/sresult.h"

#include <sys/time.h>
#include <typeinfo>
#include <stdexcept>
#include <type_traits>
#if !defined(_MSC_VER)
#include <cxxabi.h>
#endif

namespace communication {

template<typename T>
struct is_error_data  : std::enable_if<std::is_base_of<data::MessageError, T>::value, int> {};
template<typename T>
struct is_failed_data : std::enable_if<std::is_base_of<data::MessageFailed, T>::value, int> {};
template<typename T>
struct not_error_data : std::enable_if<!std::is_base_of<data::MessageError, T>::value
                                    && !std::is_base_of<data::MessageFailed, T>::value, int> {};

template <class CommandDataT>
class CheckSerializeFunc
{
    // Источник:
    // https://stackoverflow.com/questions/257288/is-it-possible-to-write-a-template-to-check-for-a-functions-existence

    // This type won't compile if the second template parameter isn't of type T,
    // so I can put a function pointer type in the first parameter and the function
    // itself in the second thus checking that the function has a specific signature.
    template <typename T, T> struct TypeCheck;

    // A helper struct to hold the declaration of the function pointer.
    // Change it if the function signature changes.
    struct FuncToRaw   {typedef bserial::RawVector (CommandDataT::*fptr)() const;};
    struct FuncFromRaw {typedef void (CommandDataT::*fptr)(const bserial::RawVector&);};

    template <typename T> static char  toRaw(TypeCheck<typename FuncToRaw::fptr, &T::toRaw>*);
    template <typename T> static void* toRaw(...);

    template <typename T> static char  fromRaw(TypeCheck<typename FuncFromRaw::fptr, &T::fromRaw>*);
    template <typename T> static void* fromRaw(...);

    struct FuncToJson   {typedef QByteArray (CommandDataT::*fptr)();};
    struct FuncFromJson {typedef SResult (CommandDataT::*fptr)(const QByteArray&);};

    template <typename T> static char  toJson(TypeCheck<typename FuncToJson::fptr, &T::toJson>*);
    template <typename T> static void* toJson(...);

    template <typename T> static char  fromJson(TypeCheck<typename FuncFromJson::fptr, &T::fromJson>*);
    template <typename T> static void* fromJson(...);

public:
    enum {ToRawExists    = (1 == sizeof(toRaw<CommandDataT>(0)))};
    enum {FromRawExists  = (1 == sizeof(fromRaw<CommandDataT>(0)))};

    enum {ToJsonExists   = (1 == sizeof(toJson<CommandDataT>(0)))};
    enum {FromJsonExists = (1 == sizeof(fromJson<CommandDataT>(0)))};
};

template<typename T>
struct toraw_exists : std::enable_if<CheckSerializeFunc<T>::ToRawExists, int> {};
template<typename T>
struct toraw_not_exists : std::enable_if<!CheckSerializeFunc<T>::ToRawExists, int> {};

template<typename T>
struct fromraw_exists : std::enable_if<CheckSerializeFunc<T>::FromRawExists, int> {};
template<typename T>
struct fromraw_not_exists : std::enable_if<!CheckSerializeFunc<T>::FromRawExists, int> {};

template<typename T>
struct tojson_exists : std::enable_if<CheckSerializeFunc<T>::ToJsonExists, int> {};
template<typename T>
struct tojson_not_exists : std::enable_if<!CheckSerializeFunc<T>::ToJsonExists, int> {};

template<typename T>
struct fromjson_exists : std::enable_if<CheckSerializeFunc<T>::FromJsonExists, int> {};
template<typename T>
struct fromjson_not_exists : std::enable_if<!CheckSerializeFunc<T>::FromJsonExists, int> {};

template<typename T> inline char* abiTypeName()
{
#if defined(_MSC_VER)
    return typeid(T).name();
#else
    return abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
}

#ifdef BPROTO_SERIALIZATION
template<typename CommandDataT>
SResult messageWriteBProto(const CommandDataT& data, Message::Ptr& message,
                           typename toraw_exists<CommandDataT>::type = 0,
                           typename is_error_data<CommandDataT>::type = 0)
{
    if (std::is_same<data::MessageError, CommandDataT>::value)
        return message->writeContent(data);

    if (std::is_base_of<error::Trait, CommandDataT>::value)
    {
        // Отладить
        break_point

        return message->writeContent(data);
    }

    // Отладить
    break_point

    return message->writeContent(static_cast<const data::MessageError&>(data), data);
}

template<typename CommandDataT>
SResult messageWriteBProto(const CommandDataT& data, Message::Ptr& message,
                           typename toraw_exists<CommandDataT>::type = 0,
                           typename is_failed_data<CommandDataT>::type = 0)
{
    if (std::is_same<data::MessageFailed, CommandDataT>::value)
        return message->writeContent(data);

    // Отладить
    break_point

    return message->writeContent(static_cast<const data::MessageFailed&>(data), data);
}

template<typename CommandDataT>
SResult messageWriteBProto(const CommandDataT& data, Message::Ptr& message,
                           typename toraw_exists<CommandDataT>::type = 0,
                           typename not_error_data<CommandDataT>::type = 0)
{
    return message->writeContent(data);
}

template<typename CommandDataT>
SResult messageWriteBProto(const CommandDataT&, Message::Ptr&,
                           typename toraw_not_exists<CommandDataT>::type = 0)
{
    QString err = "Method %1::toRaw not exists";
    err = err.arg(abiTypeName<CommandDataT>());
    log_error << err;
    return SResult(false, 0, err);
}
#endif // BPROTO_SERIALIZATION

#ifdef JSON_SERIALIZATION
template<typename CommandDataT>
SResult messageWriteJson(const CommandDataT& data, Message::Ptr& message,
                         typename tojson_exists<CommandDataT>::type = 0)
{
    return message->writeJsonContent(data);
}

template<typename CommandDataT>
SResult messageWriteJson(const CommandDataT&, Message::Ptr&,
                         typename tojson_not_exists<CommandDataT>::type = 0)
{
    QString err = "Method %1::toJson not exists";
    err = err.arg(abiTypeName<CommandDataT>());
    log_error << err;
    return SResult(false, 0, err);
}
#endif // JSON_SERIALIZATION

template<typename CommandDataT>
SResult messageWriteContent(const CommandDataT& data, Message::Ptr& message,
                            SerializationFormat contentFormat)
{
    SResult res {false};
    switch (contentFormat)
    {
#ifdef BPROTO_SERIALIZATION
        case SerializationFormat::BProto:
            res = messageWriteBProto(data, message);
            break;
#endif
#ifdef JSON_SERIALIZATION
        case SerializationFormat::Json:
            res = messageWriteJson(data, message);
            break;
#endif
        default:
        {
            log_error << "Unsupported message serialization format";
            prog_abort();
        }
    }
    return res;
}

struct CreateMessageParams
{
    const Message::Type type = {Message::Type::Command};
    const SerializationFormat format = {SerializationFormat::BProto};

    CreateMessageParams() = default;
    CreateMessageParams(Message::Type type,
                        SerializationFormat format = SerializationFormat::BProto)
        : type{type}, format{format}
    {}
    CreateMessageParams(SerializationFormat format,
                        Message::Type type = Message::Type::Command)
        : type{type}, format{format}
    {}
};

/**
  Создает сообщение на основе структуры данных соответствующей определнной
  команде. Структуры данных описаны в модулях commands_base и commands
*/
template<typename CommandDataT>
Message::Ptr createMessage(const CommandDataT& data,
                           const CreateMessageParams& params = CreateMessageParams())
{
    static_assert(CommandDataT::forCommandMessage()
                  || CommandDataT::forEventMessage(),
                  "In this function is allow 'Message::Type::Command'"
                  " or 'Message::Type::Event' type of struct only");

    Message::Ptr message = Message::create(data.command(), params.format);
    if (CommandDataT::forCommandMessage()
        && CommandDataT::forEventMessage())
    {
        if (params.type != Message::Type::Command
            && params.type != Message::Type::Event)
        {
            log_error << "Parameter 'type' must be of type 'Message::Type::Command'"
                         " or 'Message::Type::Event' only";
            prog_abort();
        }
        message->setType(params.type);
    }
    else if (CommandDataT::forCommandMessage())
        message->setType(Message::Type::Command);
    else
        message->setType(Message::Type::Event);

    message->setExecStatus(Message::ExecStatus::Unknown);
    messageWriteContent(data, message, params.format);
    return message;
}

inline Message::Ptr createMessage(const QUuidEx& command)
{
    return Message::create(command, SerializationFormat::BProto);
}

#ifdef JSON_SERIALIZATION
template<typename CommandDataT>
Message::Ptr createJsonMessage(const CommandDataT& data,
                               Message::Type type = Message::Type::Command)
{
    return createMessage(data, {type, SerializationFormat::Json});
}

inline Message::Ptr createJsonMessage(const QUuidEx& command)
{
    return Message::create(command, SerializationFormat::Json);
}
#endif

#ifdef BPROTO_SERIALIZATION
template<typename CommandDataT>
SResult messageReadBProto(const Message::Ptr& message, CommandDataT& data,
                          typename fromraw_exists<CommandDataT>::type = 0,
                          typename is_error_data<CommandDataT>::type = 0)
{
    if (std::is_same<data::MessageError, CommandDataT>::value)
        return message->readContent(data);

    // Отладить
    break_point

    return message->readContent(static_cast<data::MessageError&>(data), data);
}

template<typename CommandDataT>
SResult messageReadBProto(const Message::Ptr& message, CommandDataT& data,
                          typename fromraw_exists<CommandDataT>::type = 0,
                          typename is_failed_data<CommandDataT>::type = 0)
{
    if (std::is_same<data::MessageFailed, CommandDataT>::value)
        return message->readContent(data);

    // Отладить
    break_point

    return message->readContent(static_cast<data::MessageFailed&>(data), data);
}

template<typename CommandDataT>
SResult messageReadBProto(const Message::Ptr& message, CommandDataT& data,
                          typename fromraw_exists<CommandDataT>::type = 0,
                          typename not_error_data<CommandDataT>::type = 0)
{
    return message->readContent(data);
}

template<typename CommandDataT>
SResult messageReadBProto(const Message::Ptr&, CommandDataT&,
                          typename fromraw_not_exists<CommandDataT>::type = 0)
{
    QString err = "Method %1::fromRaw not exists";
    err = err.arg(abiTypeName<CommandDataT>());
    log_error << err;
    return SResult(false, 0, err);
}
#endif // BPROTO_SERIALIZATION

#ifdef JSON_SERIALIZATION
template<typename CommandDataT>
SResult messageReadJson(const Message::Ptr& message, CommandDataT& data,
                        typename fromjson_exists<CommandDataT>::type = 0)
{
    return message->readJsonContent(data);
}

template<typename CommandDataT>
SResult messageReadJson(const Message::Ptr&, CommandDataT&,
                          typename fromjson_not_exists<CommandDataT>::type = 0)
{
    QString err = "Method %1::fromJson not exists";
    err = err.arg(abiTypeName<CommandDataT>());
    log_error << err;
    return SResult(false, 0, err);
}
#endif // JSON_SERIALIZATION

template<typename CommandDataT>
SResult messageReadContent(const Message::Ptr& message, CommandDataT& data,
                           ErrorSenderFunc errorSender)
{
    SResult res {false};
    switch (message->contentFormat())
    {
#ifdef BPROTO_SERIALIZATION
        case SerializationFormat::BProto:
            res = messageReadBProto(message, data);
            break;
#endif
#ifdef JSON_SERIALIZATION
        case SerializationFormat::Json:
            res = messageReadJson(message, data);
            break;
#endif
        default:
            log_error << "Unsupported message serialization format";
            prog_abort();
    }

    if (!res && errorSender)
    {
        data::Error error;
        error.commandId   = message->command();
        error.messageId   = message->id();
        error.code        = error::MessageContentParse;
        error.description = res.description();
        Message::Ptr err = createMessage(error, {message->contentFormat()});
        err->destinationSocketDescriptors().insert(message->socketDescriptor());
        errorSender(err);
    }
    return res;
}

/**
  Преобразует содержимое Message-сообщения с структуру CommandDataT.
  Перед преобразованием выполняется ряд проверок, которые должны исключить
  некорректную десериализацию данных. В случае удачной десериализации поле
  CommandDataT::dataIsValid выставляется в TRUE
*/
template<typename CommandDataT>
SResult readFromMessage(const Message::Ptr& message, CommandDataT& data,
                        ErrorSenderFunc errorSender = ErrorSenderFunc())
{
    data.dataIsValid = false;

    if (message->command() != data.command())
    {
        log_error << "Command of message " << CommandNameLog(message->command())
                  << " is not equivalent command for data " << CommandNameLog(data.command());
    }
    else if (message->type() == Message::Type::Command)
    {
        if (data.forCommandMessage())
        {
            SResult res = messageReadContent(message, data, errorSender);
            data.dataIsValid = (bool)res;
            return res;
        }
        log_error << "Message " << CommandNameLog(message->command())
                  << " with type 'Command' cannot write data to struct "
                  << abiTypeName<CommandDataT>() << ". Mismatched types";
    }
    else if (message->type() == Message::Type::Event)
    {
        if (data.forEventMessage())
        {
            SResult res = messageReadContent(message, data, errorSender);
            data.dataIsValid = (bool)res;
            return res;
        }
        log_error << "Message " << CommandNameLog(message->command())
                  << " with type 'Event' cannot write data to struct "
                  << abiTypeName<CommandDataT>() << ". Mismatched types";
    }
    else if (message->type() == Message::Type::Answer)
    {
        if (message->execStatus() == Message::ExecStatus::Success)
        {
            if (data.forAnswerMessage())
            {
                SResult res = messageReadContent(message, data, errorSender);
                data.dataIsValid = (bool)res;
                return res;
            }
            log_error << "Message " << CommandNameLog(message->command())
                      << " with type 'Answer' cannot write data to struct "
                      << abiTypeName<CommandDataT>() << ". Mismatched types";
        }
        else if (message->execStatus() == Message::ExecStatus::Failed)
        {
            if (data.forAnswerMessage()
                && std::is_base_of<data::MessageFailed, CommandDataT>::value)
            {
                SResult res = messageReadContent(message, data, errorSender);
                data.dataIsValid = (bool)res;
                return res;
            }
            log_error << "Message is failed. Type of data must be "
                      << "derived from communication::data::MessageFailed"
                      << ". Command: " << CommandNameLog(message->command())
                      << ". Struct: "  << abiTypeName<CommandDataT>();
        }
        else if (message->execStatus() == Message::ExecStatus::Error)
        {
            if (data.forAnswerMessage()
                && std::is_base_of<data::MessageError, CommandDataT>::value)
            {
                SResult res = messageReadContent(message, data, errorSender);
                data.dataIsValid = (bool)res;
                return res;
            }
            log_error << "Message is error. Type of data must be "
                      << "derived from communication::data::MessageError"
                      << ". Command: " << CommandNameLog(message->command())
                      << ". Struct: "  << abiTypeName<CommandDataT>();
        }
        else
            log_error << "Message exec status is unknown: "
                      << static_cast<quint32>(message->execStatus())
                      << ". Command: " << CommandNameLog(message->command())
                      << ". Struct: "  << abiTypeName<CommandDataT>();
    }
    prog_abort();

    /* Fix warn -Wreturn-type */
    return SResult();
}

/**
  Специализированные функции для чтения сообщений MessageError, MessageFailed
*/
SResult readFromMessage(const Message::Ptr&, data::MessageError&,
                        ErrorSenderFunc errorSender = ErrorSenderFunc());

SResult readFromMessage(const Message::Ptr&, data::MessageFailed&,
                        ErrorSenderFunc errorSender = ErrorSenderFunc());

/**
  Преобразует структуру CommandDataT в Message-сообщение
*/
template<typename CommandDataT>
SResult writeToMessage(const CommandDataT& data, Message::Ptr& message,
                       SerializationFormat contentFormat = SerializationFormat::BProto,
                       typename not_error_data<CommandDataT>::type = 0)
{
    if (data.command() != message->command())
    {
        log_error << "Command of message " << CommandNameLog(message->command())
                  << " is not equal command of data " << CommandNameLog(data.command());
    }
    else if (message->type() == Message::Type::Command)
    {
        if (data.forCommandMessage())
        {
            message->setExecStatus(Message::ExecStatus::Unknown);
            return messageWriteContent(data, message, contentFormat);
        }
        log_error << "Structure of data " << abiTypeName<CommandDataT>()
                  << " cannot be used for 'Command'-message";
    }
    else if (message->type() == Message::Type::Event)
    {
        if (data.forEventMessage())
        {
            message->setExecStatus(Message::ExecStatus::Unknown);
            return messageWriteContent(data, message, contentFormat);
        }
        log_error << "Structure of data " << abiTypeName<CommandDataT>()
                  << " cannot be used for 'Event'-message";
    }
    else if (message->type() == Message::Type::Answer)
    {
        if (data.forAnswerMessage())
        {
            message->setExecStatus(Message::ExecStatus::Success);
            return messageWriteContent(data, message, contentFormat);
        }
        log_error << "Structure of data " << abiTypeName<CommandDataT>()
                  << " cannot be used for 'Answer'-message";
    }
    prog_abort();

    /* Fix warn -Wreturn-type */
    return SResult();
}

/**
  Специализированные функции для записи сообщений MessageError, MessageFailed.
  При записи данных тип сообщения меняется на Message::Type::Answer, а статус
  Message::ExecStatus на соответствующий структуре данных
*/
template<typename CommandDataT /*MessageError*/>
SResult writeToMessage(const CommandDataT& data, Message::Ptr& message,
                       SerializationFormat contentFormat = SerializationFormat::BProto,
                       typename is_error_data<CommandDataT>::type = 0)
{
    message->setType(Message::Type::Answer);
    message->setExecStatus(Message::ExecStatus::Error);
    return messageWriteContent(data, message, contentFormat);
}

template<typename CommandDataT /*MessageFailed*/>
SResult writeToMessage(const CommandDataT& data, Message::Ptr& message,
                       SerializationFormat contentFormat = SerializationFormat::BProto,
                       typename is_failed_data<CommandDataT>::type = 0)
{
    message->setType(Message::Type::Answer);
    message->setExecStatus(Message::ExecStatus::Failed);
    return messageWriteContent(data, message, contentFormat);
}

#ifdef JSON_SERIALIZATION
template<typename CommandDataT>
SResult writeToJsonMessage(const CommandDataT& data, Message::Ptr& message)
{
    return writeToMessage(data, message, SerializationFormat::Json);
}
#endif

/**
  Сервисная функция, возвращает описание ошибки из сообщений содержащих струк-
  туры MessageError, MessageFailed.  Если  сообщение  не  содержит  информации
  об ошибке - возвращается пустая строка
*/
QString errorDescription(const Message::Ptr&);

namespace data {
QDataStream& operator>> (QDataStream&, timeval&);
QDataStream& operator<< (QDataStream&, const timeval&);
} // namespace data

/**
  Выполняет проверку пересечения диапазонов версий  бинарного  протокола.
  Если диапазоны не пересекаются, то считаем, что протоколы не совместимы
*/
bool protocolCompatible(quint16 versionLow, quint16 versionHigh);

/**
  Функция регистрации Qt-метатипов для работы с коммуникационными механизмами
*/
void registrationQtMetatypes();

} // namespace communication
