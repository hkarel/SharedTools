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

#pragma once

#include "qt/bserialize.h"
#include <QtCore>
#include <QHostAddress>

namespace communication {

/**
  Структура группирует адрес и порт
*/
class HostPoint
{
public:
    typedef QSet<HostPoint> Set;

    HostPoint() = default;
    HostPoint(const HostPoint&) = default;
    HostPoint(const QHostAddress& address, int port);

    HostPoint& operator= (const HostPoint&) = default;
    bool operator== (const HostPoint&) const;
    bool isNull() const;
    void reset();

    QHostAddress address() const {return _address;}
    void setAddress(const QHostAddress& val){_address = val;}

    quint16 port() const {return _port;}
    void setPort(int val);

private:
    QHostAddress _address;
    quint16 _port = {0};

    // Функции сериализации данных
    DECLARE_B_SERIALIZE_FUNC
};
uint qHash(const HostPoint&);

} // namespace communication
