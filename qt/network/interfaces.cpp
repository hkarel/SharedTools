/* clang-format off */
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

#include "interfaces.h"
#include "logger/logger.h"
#include "qt/logger_operators.h"

#include <QNetworkAddressEntry>
#include <QNetworkInterface>

#define log_error_m   alog::logger().error   (alog_line_location, "NetIntf")
#define log_warn_m    alog::logger().warn    (alog_line_location, "NetIntf")
#define log_info_m    alog::logger().info    (alog_line_location, "NetIntf")
#define log_verbose_m alog::logger().verbose (alog_line_location, "NetIntf")
#define log_debug_m   alog::logger().debug   (alog_line_location, "NetIntf")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "NetIntf")

namespace network {

Interface::Interface()
{}

bool Interface::canBroadcast() const
{
    return (_flags & QNetworkInterface::CanBroadcast);
}

bool Interface::isPointToPoint() const
{
    return (_flags & QNetworkInterface::IsPointToPoint);
}

Interface::List getInterfaces()
{
    Interface::List interfaces;
    for (const QNetworkInterface& netIntf : QNetworkInterface::allInterfaces())
    {
        if (netIntf.flags() & QNetworkInterface::IsLoopBack)
            continue;

        if (netIntf.flags() & QNetworkInterface::IsRunning)
            for (const QNetworkAddressEntry& entry : netIntf.addressEntries())
            {
                if (entry.broadcast() == QHostAddress::Null)
                    continue;

                Interface* intf = interfaces.add();
                intf->_ip = entry.ip();
                intf->_broadcast = entry.broadcast();
                intf->_name = netIntf.name();
                intf->_flags = netIntf.flags();

                // Дебильный способ получения адреса сети, но к сожалению
                // Qt API другой возможности не предоставляет.
                QString subnetStr = entry.ip().toString()
                                    + QString("/%1").arg(entry.prefixLength());
                QPair<QHostAddress, int> subnet = QHostAddress::parseSubnet(subnetStr);
                intf->_subnet = subnet.first;
                intf->_subnetPrefixLength = entry.prefixLength();

                if (alog::logger().level() == alog::Level::Debug2)
                {
                    log_debug2_m << "Found interface: " << intf->name()
                                 << "; ip: " << intf->ip()
                                 << "; subnet: " << intf->subnet()
                                 << "; broadcast: " << intf->broadcast()
                                 << "; prefix length: " << intf->subnetPrefixLength()
                                 << "; flags: " << intf->flags()
                                 << "; canBroadcast: " << intf->canBroadcast()
                                 << "; isPointToPoint: " << intf->isPointToPoint();
                }
            }
    }
    return interfaces;
}

} // namespace network
