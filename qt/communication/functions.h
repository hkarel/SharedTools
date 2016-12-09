/*****************************************************************************
  В модуле собраны функции общего назначения для работы с коммуникационными
  механизмами.
*****************************************************************************/

#pragma once

#include "commands_base.h"
#include "message.h"

#include <sys/time.h>
#include <typeinfo>
#include <stdexcept>
#include <type_traits>

namespace communication {

QString toString(command::Type);
QString toString(command::ExecStatus);

/**
  Создает сообщение на основе структуры данных соответствующей определнной
  команде. Структуры данных описаны в модулях commands_base и commands.
*/
template<typename CommandDataT>
Message::Ptr createMessage(const CommandDataT& data,
                           command::Type type = command::Type::Request)
{
    static_assert(CommandDataT::forRequest() || CommandDataT::forEvent(),
                  "In this function is allow 'command::Type::Request'"
                  " or 'command::Type::Event' type of struct only");

    Message::Ptr m = Message::create(data.command());
    if (CommandDataT::forRequest() && CommandDataT::forEvent())
    {
        if (type != command::Type::Request
            && type != command::Type::Event)
            throw std::logic_error(std::string(
                "Parameter 'type' must be of type 'command::Type::Request'"
                " or 'command::Type::Event' only"));
        m->setCommandType(type);
    }
    else if (CommandDataT::forRequest())
        m->setCommandType(command::Type::Request);
    else if (CommandDataT::forEvent())
        m->setCommandType(command::Type::Event);

    m->setCommandExecStatus(command::ExecStatus::Unknown);
    m->writeContent(data);
    return std::move(m);
}

/**
  Преобразует содержимое Message-сообщения с структуру CommandDataT.
  Перед преобразованием выполняется ряд проверок, которые должны исключить
  некорректную десериализацию данных. В случае удачной десериализации поле
  CommandDataT::isValid выставляется в TRUE.
*/
template<typename CommandDataT>
void readFromMessage(const Message::Ptr& message, CommandDataT& data)
{
    bool res;
    QString err;
    data.isValid = false;

    if (message->command() != data.command())
    {
        err = QString("Command identifier of message (%1) is not equal "
                      "a command identifier of data (%2).")
                      .arg(message->command().toString(), data.command().toString());
    }
    else if (message->commandType() == command::Type::Request)
    {
        if (data.forRequest())
        {
            res = message->readContent(data);
            data.isValid = res;
            return;
        }
        err  = QString("Message (%1) with type 'Request' cannot write data to struct (%2)."
                       " Mismatched types.")
                       .arg((message->command().toString()), typeid(CommandDataT).name());
    }
    else if (message->commandType() == command::Type::Event)
    {
        if (data.forEvent())
        {
            res = message->readContent(data);
            data.isValid = res;
            return;
        }
        err  = QString("Message (%1) with type 'Event' cannot write data to struct (%2)."
                       " Mismatched types.")
                       .arg((message->command().toString()), typeid(CommandDataT).name());
    }
    else if (message->commandType() == command::Type::Response)
    {
        if (message->commandExecStatus() == command::ExecStatus::Success)
        {
            if (data.forResponse())
            {
                res = message->readContent(data);
                data.isValid = res;
                return;
            }
            err  = QString("Message (%1) with type 'Response' cannot write data to struct (%2)."
                           " Mismatched types.")
                           .arg((message->command().toString()), typeid(CommandDataT).name());
        }
        else if (message->commandExecStatus() == command::ExecStatus::Failed
                 && typeid(CommandDataT) != typeid(data::MessageFailed))
        {
            err = "Message is failed. Type of data must be "
                  "communication::data::MessageFailed.";
        }
        else if (message->commandExecStatus() == command::ExecStatus::Error
                 && typeid(CommandDataT) == typeid(data::MessageError))
        {
            err = "Message is error. Type of data must be "
                  "communication::data::MessageError.";
        }
        else
            err = "Message exec status is unknown.";
    }
    err += " Read the message is imposible";
    throw std::logic_error(std::string(err.toUtf8().constData()));
}

/**
  Специализированные функции для чтения сообщений MessageError, MessageFailed.
*/
void readFromMessage(const Message::Ptr&, data::MessageError&);
void readFromMessage(const Message::Ptr&, data::MessageFailed&);


/**
  Преобразует структуру CommandDataT в Message-сообщение.
*/
template<typename CommandDataT>
bool writeToMessage(const CommandDataT& data, Message::Ptr& message)
{
    QString err;
    if (data.command() != message->command())
    {
        err = QString("Command identifier of data (%1) is not equal "
                      "a command identifier of message (%2).")
                      .arg(data.command().toString(), message->command().toString());
    }
    else if (message->commandType() == command::Type::Request)
    {
        if (data.forRequest())
        {
            message->setCommandExecStatus(command::ExecStatus::Unknown);
            return message->writeContent(data);
        }
        err = "Structure of data cannot be used for 'Request'-message.";
    }
    else if (message->commandType() == command::Type::Event)
    {
        if (data.forEvent())
        {
            message->setCommandExecStatus(command::ExecStatus::Unknown);
            return message->writeContent(data);
        }
        err = "Structure of data cannot be used for 'Event'-message.";
    }
    else if (message->commandType() == command::Type::Response)
    {
        if (data.forResponse())
        {
            message->setCommandExecStatus(command::ExecStatus::Success);
            return message->writeContent(data);
        }
        err = "Structure of data cannot be used for 'Responce'-message.";
    }
    err += " Write the data is imposible";
    throw std::logic_error(std::string(err.toUtf8().constData()));
}

/**
  Специализированные функции для записи сообщений MessageError, MessageFailed.
  При записи данных тип сообщения меняется на Message::Type::Responce,
  а Message::ExecStatus на соответствующий структуре данных.
  Примечание: константный модификатор во втором параметре относится к структуре
  интеллектуального указателя Message::Ptr, но не к самому Message-сообщению.
*/
bool writeToMessage(const data::MessageError&,  Message::Ptr&);
bool writeToMessage(const data::MessageFailed&, Message::Ptr&);

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

} // namespace communication
