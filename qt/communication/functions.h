/*****************************************************************************
  В модуле собраны функции общего назначения для работы с коммуникационными
  механизмами.
*****************************************************************************/

#pragma once

#include "container_ptr.h"
#include "qt/communication/commands_base.h"
#include "qt/communication/message.h"

#include <sys/time.h>
#include <typeinfo>
#include <stdexcept>
#include <type_traits>

#include <QList>
#include <QHostAddress>

namespace communication {

//QString toString(Message::Type);
//QString toString(Message::ExecStatus);
typedef container_ptr<QList<QHostAddress>> NetAddressesPtr;

/**
  Создает сообщение на основе структуры данных соответствующей определнной
  команде. Структуры данных описаны в модулях commands_base и commands.
*/
template<typename CommandDataT>
Message::Ptr createMessage(const CommandDataT& data,
                           Message::Type type = Message::Type::Command)
{
    static_assert(CommandDataT::forCommandMessage() || CommandDataT::forEventMessage(),
                  "In this function is allow 'Message::Type::Command'"
                  " or 'Message::Type::Event' type of struct only");

    Message::Ptr m = Message::create(data.command());
    if (CommandDataT::forCommandMessage() && CommandDataT::forEventMessage())
    {
        if (type != Message::Type::Command
            && type != Message::Type::Event)
            throw std::logic_error(std::string(
                "Parameter 'type' must be of type 'Message::Type::Command'"
                " or 'Message::Type::Event' only"));
        m->setType(type);
    }
    else if (CommandDataT::forCommandMessage())
        m->setType(Message::Type::Command);
    else if (CommandDataT::forEventMessage())
        m->setType(Message::Type::Event);

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
  Возвращает список адресов для доступных на данный момент сетевых интерфейсов.
  Адрес для интерфейса localhost не возвращается.
*/
NetAddressesPtr interfacesAddresses();

/**
  Возвращает список широковещательных адресов для доступных на данный момент
  сетевых интерфейсов.
*/
NetAddressesPtr broadcastAddresses();

} // namespace communication
