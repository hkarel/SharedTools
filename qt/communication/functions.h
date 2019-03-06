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

namespace communication {

template<typename CommandDataT>
SResult messageWrite(const CommandDataT& data, Message::Ptr& message,
                     SerializationFormat contentFormat)
{
    SResult res {false};
    switch (contentFormat)
    {
        case SerializationFormat::BProto:
            res = message->writeContent(data);
            break;
#ifdef JSON_SERIALIZATION
        case SerializationFormat::Json:
            res = message->writeJsonContent(data);
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
  команде. Структуры данных описаны в модулях commands_base и commands.
*/
template<typename CommandDataT>
Message::Ptr createMessage(const CommandDataT& data,
                           const CreateMessageParams& params = CreateMessageParams())
{
    static_assert(CommandDataT::forCommandMessage()
                  || CommandDataT::forEventMessage(),
                  "In this function is allow 'Message::Type::Command'"
                  " or 'Message::Type::Event' type of struct only");

    Message::Ptr m = Message::create(data.command());
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
        m->setType(params.type);
    }
    else if (CommandDataT::forCommandMessage())
        m->setType(Message::Type::Command);
    else
        m->setType(Message::Type::Event);

    m->setExecStatus(Message::ExecStatus::Unknown);
    messageWrite(data, m, params.format);
    return std::move(m);
}

inline Message::Ptr createMessage(const QUuidEx& command)
{
    return Message::create(command);
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
    return Message::create(command);
}
#endif

template<typename CommandDataT>
SResult messageRead(const Message::Ptr& message, CommandDataT& data,
                    ErrorSenderFunc errorSender)
{
    SResult res {false};
    switch (message->contentFormat())
    {
        case SerializationFormat::BProto:
            res = message->readContent(data);
            break;
#ifdef JSON_SERIALIZATION
        case SerializationFormat::Json:
            res = message->readJsonContent(data);
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
  CommandDataT::dataIsValid выставляется в TRUE.
*/
template<typename CommandDataT>
SResult readFromMessage(const Message::Ptr& message, CommandDataT& data,
                        ErrorSenderFunc errorSender = ErrorSenderFunc())
{
    data.dataIsValid = false;

    if (message->command() != data.command())
    {
        log_error << "Command of message " << CommandNameLog(message->command())
                  << " is not equal command of data " << CommandNameLog(data.command());
    }
    else if (message->type() == Message::Type::Command)
    {
        if (data.forCommandMessage())
        {
            SResult res = messageRead(message, data, errorSender);
            data.dataIsValid = (bool)res;
            return res;
        }
        log_error << "Message " << CommandNameLog(message->command())
                  << " with type 'Command' cannot write data to struct "
                  << typeid(CommandDataT).name() << ". Mismatched types";
    }
    else if (message->type() == Message::Type::Event)
    {
        if (data.forEventMessage())
        {
            SResult res = messageRead(message, data, errorSender);
            data.dataIsValid = (bool)res;
            return res;
        }
        log_error << "Message " << CommandNameLog(message->command())
                  << " with type 'Event' cannot write data to struct "
                  << typeid(CommandDataT).name() << ". Mismatched types";
    }
    else if (message->type() == Message::Type::Answer)
    {
        if (message->execStatus() == Message::ExecStatus::Success)
        {
            if (data.forAnswerMessage())
            {
                SResult res = messageRead(message, data, errorSender);
                data.dataIsValid = (bool)res;
                return res;
            }
            log_error << "Message " << CommandNameLog(message->command())
                      << " with type 'Answer' cannot write data to struct "
                      << typeid(CommandDataT).name() << ". Mismatched types";
        }
        else if (message->execStatus() == Message::ExecStatus::Failed
                 && typeid(CommandDataT) != typeid(data::MessageFailed))
        {
            log_error << "Message is failed. Type of data must be "
                         "communication::data::MessageFailed";
        }
        else if (message->execStatus() == Message::ExecStatus::Error
                 && typeid(CommandDataT) == typeid(data::MessageError))
        {
            log_error << "Message is error. Type of data must be "
                         "communication::data::MessageError";
        }
        else
            log_error << "Message exec status is unknown";
    }
    prog_abort();

    /* Фикс варнинга -Wreturn-type */
    return SResult();
}

/**
  Специализированные функции для чтения сообщений MessageError, MessageFailed.
*/
SResult readFromMessage(const Message::Ptr&, data::MessageError&,
                        ErrorSenderFunc errorSender = ErrorSenderFunc());

SResult readFromMessage(const Message::Ptr&, data::MessageFailed&,
                        ErrorSenderFunc errorSender = ErrorSenderFunc());

template<typename T>
struct is_error_data : std::enable_if<std::is_base_of<error::Trait, T>::value, int> {};
template<typename T>
struct not_error_data : std::enable_if<!std::is_base_of<error::Trait, T>::value, int> {};

/**
  Преобразует структуру CommandDataT в Message-сообщение.
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
            return messageWrite(data, message, contentFormat);
        }
        log_error << "Structure of data " << typeid(CommandDataT).name()
                  << " cannot be used for 'Command'-message";
    }
    else if (message->type() == Message::Type::Event)
    {
        if (data.forEventMessage())
        {
            message->setExecStatus(Message::ExecStatus::Unknown);
            return messageWrite(data, message, contentFormat);
        }
        log_error << "Structure of data " << typeid(CommandDataT).name()
                  << " cannot be used for 'Event'-message";
    }
    else if (message->type() == Message::Type::Answer)
    {
        if (data.forAnswerMessage())
        {
            message->setExecStatus(Message::ExecStatus::Success);
            return messageWrite(data, message, contentFormat);
        }
        log_error << "Structure of data " << typeid(CommandDataT).name()
                  << " cannot be used for 'Answer'-message";
    }
    prog_abort();
}

/**
  Специализированные функции для записи сообщений MessageError, MessageFailed.
  При записи данных тип сообщения меняется на Message::Type::Responce,
  а Message::ExecStatus на соответствующий структуре данных.
*/
SResult writeToMessage(const data::MessageError&,  Message::Ptr&,
                       SerializationFormat = SerializationFormat::BProto);

SResult writeToMessage(const data::MessageFailed&, Message::Ptr&,
                       SerializationFormat = SerializationFormat::BProto);

template<typename ErrorT>
SResult writeToMessage(const ErrorT& data, Message::Ptr& message,
                       SerializationFormat contentFormat = SerializationFormat::BProto,
                       typename is_error_data<ErrorT>::type = 0)
{
    return writeToMessage(data.asError(), message, contentFormat);
}

#ifdef JSON_SERIALIZATION
template<typename CommandDataT>
SResult writeToJsonMessage(const CommandDataT& data, Message::Ptr& message)
{
    return writeToMessage(data, message, SerializationFormat::Json);
}
#endif

/**
  Сервисная функция, возвращает описание ошибки из сообщений содержащих
  структуры MessageError, MessageFailed. Если сообщение не содержит информации
  об ошибке - возвращается пустая строка.
*/
QString errorDescription(const Message::Ptr&);

namespace data {
QDataStream& operator>> (QDataStream&, timeval&);
QDataStream& operator<< (QDataStream&, const timeval&);
} // namespace data

/**
  Выполняет проверку пересечения диапазонов версий бинарного протокола.
  Если диапазоны не пересекаются, то считаем, что протоколы не совместимы.
*/
bool protocolCompatible(quint16 versionLow, quint16 versionHigh);

/**
  Функция регистрации Qt-метатипов для работы с коммуникационными механизмами.
*/
void registrationQtMetatypes();

} // namespace communication
