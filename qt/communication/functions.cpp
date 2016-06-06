#include "functions.h"

namespace communication {

QString toString(command::Type t)
{
    switch (t)
    {
        case command::Type::Request:  return "Request";
        case command::Type::Response: return "Response";
        default: return "Request";
    }
}

QString toString(Message::Type t)
{
    switch (t)
    {
        case Message::Type::Request:  return "Request";
        case Message::Type::Responce: return "Responce";
        case Message::Type::Event:    return "Event";
        default: return "Request";
    }
}

QString toString(Message::ExecStatus e)
{
    switch (e)
    {
        case Message::ExecStatus::Success: return "Success";
        case Message::ExecStatus::Failed:  return "Failed";
        case Message::ExecStatus::Error:   return "Error";
        default: return "Unknown";
    }
}

void readFromMessage(const Message::Ptr& message, data::MessageError& data)
{
    QString err;
    if (message->type() == Message::Type::Responce)
    {
        if (message->execStatus() == Message::ExecStatus::Error)
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
    if (message->type() == Message::Type::Responce)
    {
        if (message->execStatus() == Message::ExecStatus::Failed)
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
    message->setType(Message::Type::Responce);
    message->setExecStatus(Message::ExecStatus::Error);
    return message->writeContent(data);
}

bool writeToMessage(const data::MessageFailed& data, Message::Ptr& message)
{
    message->setType(Message::Type::Responce);
    message->setExecStatus(Message::ExecStatus::Failed);
    return message->writeContent(data);
}

QString errorDescription(const Message::Ptr& message)
{
    QString descr;
    if (message->type() == Message::Type::Responce)
    {
        if (message->execStatus() == Message::ExecStatus::Failed)
        {
            data::MessageFailed data;
            readFromMessage(message, data);
            descr = data.description;
        }
        else if (message->execStatus() == Message::ExecStatus::Error)
        {
            data::MessageError data;
            readFromMessage(message, data);
            descr = data.description;
        }
    }
    return descr;
}

} // namespace communication
