/*****************************************************************************
  The MIT License

  Copyright © 2020 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "qt/communication/serialize/functions.h"

#define log_error_m   alog::logger().error  (__FILE__, __func__, __LINE__, "Serialize")
#define log_warn_m    alog::logger().warn   (__FILE__, __func__, __LINE__, "Serialize")
#define log_info_m    alog::logger().info   (__FILE__, __func__, __LINE__, "Serialize")
#define log_verbose_m alog::logger().verbose(__FILE__, __func__, __LINE__, "Serialize")
#define log_debug_m   alog::logger().debug  (__FILE__, __func__, __LINE__, "Serialize")
#define log_debug2_m  alog::logger().debug2 (__FILE__, __func__, __LINE__, "Serialize")

namespace communication {

SResult readFromMessage(const Message::Ptr& message, data::MessageError& data,
                        ErrorSenderFunc errorSender)
{
    if (message->type() != Message::Type::Answer)
    {
        log_error_m << "Message type must be Message::Type::Answer";
        prog_abort();
    }
    if (message->execStatus() != Message::ExecStatus::Error)
    {
        log_error_m << "Message exec status must be Message::ExecStatus::Error";
        prog_abort();
    }
    return detail::messageReadContent(message, data, errorSender);
}

SResult readFromMessage(const Message::Ptr& message, data::MessageFailed& data,
                        ErrorSenderFunc errorSender)
{
    if (message->type() != Message::Type::Answer)
    {
        log_error_m << "Message type must be Message::Type::Answer";
        prog_abort();
    }
    if (message->execStatus() != Message::ExecStatus::Failed)
    {
        log_error_m << "Message exec status must be Message::ExecStatus::Failed";
        prog_abort();
    }
    return detail::messageReadContent(message, data, errorSender);
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

} // namespace communication

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
