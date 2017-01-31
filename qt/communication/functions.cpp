#include "qt/communication/functions.h"
#include <QNetworkInterface>

namespace communication {

//QString toString(Message::Type t)
//{
//    switch (t)
//    {
//        case Message::Type::Command:  return "Command";
//        case Message::Type::Answer:   return "Answer";
//        case Message::Type::Event:    return "Event";
//        default:                      return "Unknown";
//    }
//}

//QString toString(Message::ExecStatus e)
//{
//    switch (e)
//    {
//        case command::ExecStatus::Success: return "Success";
//        case command::ExecStatus::Failed:  return "Failed";
//        case command::ExecStatus::Error:   return "Error";
//        default:                           return "Unknown";
//    }
//}

void readFromMessage(const Message::Ptr& message, data::MessageError& data)
{
    QString err;
    if (message->type() == Message::Type::Answer)
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
    if (message->type() == Message::Type::Answer)
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
    message->setType(Message::Type::Answer);
    message->setExecStatus(Message::ExecStatus::Error);
    return message->writeContent(data);
}

bool writeToMessage(const data::MessageFailed& data, Message::Ptr& message)
{
    message->setType(Message::Type::Answer);
    message->setExecStatus(Message::ExecStatus::Failed);
    return message->writeContent(data);
}

QString errorDescription(const Message::Ptr& message)
{
    QString descr;
    if (message->type() == Message::Type::Answer)
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

namespace data {
QDataStream& operator>> (QDataStream& s, timeval& tv)
{
    qint64 sec;
    qint32 usec;
    s >> sec;
    s >> usec;
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    return s;
}

QDataStream& operator<< (QDataStream& s, const timeval& tv)
{
    s << qint64(tv.tv_sec);
    s << qint32(tv.tv_usec);
    return s;
}
} // namespace data

bool protocolCompatible(quint16 versionLow, quint16 versionHigh)
{
    if (versionLow > versionHigh
        || BPROTOCOL_VERSION_LOW > BPROTOCOL_VERSION_HIGH)
        return false;
    quint16 protocolVersionLow = BPROTOCOL_VERSION_LOW;
    if (versionHigh < protocolVersionLow)
        return false;
    if (versionLow > BPROTOCOL_VERSION_HIGH)
        return false;
    return true;
}

NetAddressesPtr netAddresses(bool returnBroadcast)
{
    NetAddressesPtr la = NetAddressesPtr::create_ptr();

    for (const QNetworkInterface& interface : QNetworkInterface::allInterfaces())
        for (const QNetworkAddressEntry& entry : interface.addressEntries())
        {
            QHostAddress address = entry.ip();
            QHostAddress broadcast = entry.broadcast();
            if (address != QHostAddress::LocalHost
                && broadcast != QHostAddress::Null)
            {
                if (returnBroadcast)
                    la->append(broadcast);
                else
                    la->append(address);
            }
        }

    return std::move(la);
}

NetAddressesPtr interfacesAddresses()
{
    return netAddresses(false);
}

NetAddressesPtr broadcastAddresses()
{
    return netAddresses(true);
}

} // namespace communication
