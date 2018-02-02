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
*****************************************************************************/

#include "qt/communication/functions.h"

namespace communication {

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

void registrationQtMetatypes()
{
    static bool first {true};
    if (first)
    {
        qRegisterMetaType<communication::Message::Ptr>("communication::Message::Ptr");
        qRegisterMetaType<communication::SocketDescriptor>("communication::SocketDescriptor");
        first = false;
    }
}

} // namespace communication
