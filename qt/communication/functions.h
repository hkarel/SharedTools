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

#include "qt/communication/commands_base.h"
#include "qt/communication/message.h"

#include <sys/time.h>
#include <typeinfo>
#include <stdexcept>
#include <type_traits>

namespace communication {

//QString toString(Message::Type);
//QString toString(Message::ExecStatus);
//typedef container_ptr<QList<QHostAddress>> NetAddressesPtr;

/**
  Создает сообщение на основе структуры данных соответствующей определнной
  команде. Структуры данных описаны в модулях commands_base и commands.
*/
template<typename CommandDataT>
Message::Ptr createMessage(const CommandDataT& data,
                           Message::Type type = Message::Type::Command)
{
    static_assert(CommandDataT::forCommandMessage()
                  || CommandDataT::forEventMessage(),
                  "In this function is allow 'Message::Type::Command'"
                  " or 'Message::Type::Event' type of struct only");

    Message::Ptr m = Message::create(data.command());
    if (CommandDataT::forCommandMessage()
        && CommandDataT::forEventMessage())
    {
        if (type != Message::Type::Command
            && type != Message::Type::Event)
            throw std::logic_error(
                "Parameter 'type' must be of type 'Message::Type::Command'"
                " or 'Message::Type::Event' only");
        m->setType(type);
    }
    else if (CommandDataT::forCommandMessage())
        m->setType(Message::Type::Command);
    else
        m->setType(Message::Type::Event);

    m->setExecStatus(Message::ExecStatus::Unknown);
    m->writeContent(data);
    return std::move(m);
}

inline Message::Ptr createMessage(const QUuidEx& command)
{
    return Message::create(command);
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
    else if (message->type() == Message::Type::Command)
    {
        if (data.forCommandMessage())
        {
            res = message->readContent(data);
            data.isValid = res;
            return;
        }
        err  = QString("Message (%1) with type 'Command' cannot write data to struct (%2)."
                       " Mismatched types.")
                       .arg((message->command().toString()), typeid(CommandDataT).name());
    }
    else if (message->type() == Message::Type::Event)
    {
        if (data.forEventMessage())
        {
            res = message->readContent(data);
            data.isValid = res;
            return;
        }
        err  = QString("Message (%1) with type 'Event' cannot write data to struct (%2)."
                       " Mismatched types.")
                       .arg((message->command().toString()), typeid(CommandDataT).name());
    }
    else if (message->type() == Message::Type::Answer)
    {
        if (message->execStatus() == Message::ExecStatus::Success)
        {
            if (data.forAnswerMessage())
            {
                res = message->readContent(data);
                data.isValid = res;
                return;
            }
            err  = QString("Message (%1) with type 'Answer' cannot write data to struct (%2)."
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
    else if (message->type() == Message::Type::Command)
    {
        if (data.forCommandMessage())
        {
            message->setExecStatus(Message::ExecStatus::Unknown);
            return message->writeContent(data);
        }
        err = "Structure of data cannot be used for 'Request'-message.";
    }
    else if (message->type() == Message::Type::Event)
    {
        if (data.forEventMessage())
        {
            message->setExecStatus(Message::ExecStatus::Unknown);
            return message->writeContent(data);
        }
        err = "Structure of data cannot be used for 'Event'-message.";
    }
    else if (message->type() == Message::Type::Answer)
    {
        if (data.forAnswerMessage())
        {
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
