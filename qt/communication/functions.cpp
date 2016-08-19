#include "functions.h"

namespace communication {

QString toString(command::Type t)
{
    switch (t)
    {
        case command::Type::Request:  return "Request";
        case command::Type::Response: return "Response";
        case command::Type::Event:    return "Event";
        default:                      return "Unknown";
    }
}

QString toString(command::ExecStatus e)
{
    switch (e)
    {
        case command::ExecStatus::Success: return "Success";
        case command::ExecStatus::Failed:  return "Failed";
        case command::ExecStatus::Error:   return "Error";
        default:                           return "Unknown";
    }
}

void readFromMessage(const Message::Ptr& message, data::MessageError& data)
{
    QString err;
    if (message->commandType() == command::Type::Response)
    {
        if (message->commandExecStatus() == command::ExecStatus::Error)
        {
            message->readContent(data);
            return;
        }
        err = "Message exec status must be Message::ExecStatus::Error.";
    }
    else
        err = "Message type must be Message::Type::Responce.";

    err += " Read the message is imposible";
    throw std::logic_error(std::string(err.toUtf8().constData()));
}

void readFromMessage(const Message::Ptr& message, data::MessageFailed& data)
{
    QString err;
    if (message->commandType() == command::Type::Response)
    {
        if (message->commandExecStatus() == command::ExecStatus::Failed)
        {
            message->readContent(data);
            return;
        }
        err = "Message exec status must be Message::ExecStatus::Failed.";
    }
    else
        err = "Message type must be Message::Type::Responce.";

    err += " Read the message is imposible";
    throw std::logic_error(std::string(err.toUtf8().constData()));
}

bool writeToMessage(const data::MessageError& data, Message::Ptr& message)
{
    message->setCommandType(command::Type::Response);
    message->setCommandExecStatus(command::ExecStatus::Error);
    return message->writeContent(data);
}

bool writeToMessage(const data::MessageFailed& data, Message::Ptr& message)
{
    message->setCommandType(command::Type::Response);
    message->setCommandExecStatus(command::ExecStatus::Failed);
    return message->writeContent(data);
}

QString errorDescription(const Message::Ptr& message)
{
    QString descr;
    if (message->commandType() == command::Type::Response)
    {
        if (message->commandExecStatus() == command::ExecStatus::Failed)
        {
            data::MessageFailed data;
            readFromMessage(message, data);
            descr = data.description;
        }
        else if (message->commandExecStatus() == command::ExecStatus::Error)
        {
            data::MessageError data;
            readFromMessage(message, data);
            descr = data.description;
        }
    }
    return descr;
}

} // namespace communication
