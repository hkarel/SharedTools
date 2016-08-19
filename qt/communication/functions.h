/*****************************************************************************
  В модуле собраны функции общего назначения для работы с коммуникационными
  механизмами.
*****************************************************************************/

#pragma once

#include "commands_base.h"
#include "message.h"

#include <typeinfo>
#include <stdexcept>
#include <type_traits>

namespace communication {

QString toString(command::Type);
QString toString(Message::ExecStatus);

/**
  Создает сообщение на основе структуры данных соответствующей определнной
  команде. Структуры данных описаны в модулях commands_base и commands.
*/
template<typename CommandDataT>
Message::Ptr createMessage(const CommandDataT& data)
{
    static_assert(CommandDataT::forRequest() || CommandDataT::forEvent(),
                  "In this function is allow 'command::Type::Request'"
                  " or 'command::Type::Event' type of struct only");

    Message::Ptr m = Message::create(data.command());
    if (CommandDataT::forRequest())
        m->setCommandType(command::Type::Request);
    else if (CommandDataT::forEvent())
        m->setCommandType(command::Type::Event);

    m->setExecStatus(Message::ExecStatus::Unknown);
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
        if (message->execStatus() == Message::ExecStatus::Success)
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
        else if (message->execStatus() == Message::ExecStatus::Failed
                 && typeid(CommandDataT) != typeid(data::MessageFailed))
        {
            err = "Message is failed. Type of data must be "
                  "communication::data::MessageFailed.";
        }
        else if (message->execStatus() == Message::ExecStatus::Error
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
    else if (message->commandType() == command::Type::Request
             || message->commandType() == command::Type::Event)
    {
        if (data.forRequest())
        {
            message->setCommandType(command::Type::Request);
            message->setExecStatus(Message::ExecStatus::Unknown);
            return message->writeContent(data);
        }
        err = "Structure of data cannot be used for 'Request'-message.";
    }
    else if (message->commandType() == command::Type::Response)
    {
        if (data.forResponse())
        {
            message->setCommandType(command::Type::Response);
            message->setExecStatus(Message::ExecStatus::Success);
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
*/
bool writeToMessage(const data::MessageError&,  Message::Ptr&);
bool writeToMessage(const data::MessageFailed&, Message::Ptr&);

/**
  Сервисная функция, возвращает описание ошибки из сообщений содержащих
  структуры MessageError, MessageFailed. Если сообщение не содержит информации
  об ошибке - возвращается пустая строка.
*/
QString errorDescription(const Message::Ptr&);

} // namespace communication
