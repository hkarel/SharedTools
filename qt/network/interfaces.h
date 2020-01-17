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

#pragma once

#include "shared/list.h"
#include "shared/clife_base.h"
#include "shared/clife_alloc.h"
#include "shared/clife_ptr.h"

#include <QtCore>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkAccessManager>

namespace network {

/**
  Структура содержит совокупные характеристики сетевого интерфейса
*/
class Interface : public clife_base
{
public:
    typedef clife_ptr<Interface> Ptr;

    Interface();

    // Адрес сетевого интерфейса
    QHostAddress ip() const {return _ip;}

    // Адрес сегмента сети
    QHostAddress subnet() const {return _subnet;}

    // Широковещательный адрес сетевого интерфейса
    QHostAddress broadcast() const {return _broadcast;}

    // Длина маски подсети
    int	subnetPrefixLength() const {return _subnetPrefixLength;}

    // Наименование сетевого интерфейса
    QString name() const {return _name;}

    // Флаги ассоциированные с сетевым интерфейсом
    quint32 flags() const {return _flags;}

    // Интерфейс может работать в broadcast режиме
    bool canBroadcast() const;

    // Интерфейс работает в режиме point-to-point
    bool isPointToPoint() const;

    typedef lst::List<Interface, lst::CompareItemDummy, clife_alloc<Interface>> List;

private:
    QHostAddress _ip;
    QHostAddress _subnet;
    QHostAddress _broadcast;
    int	         _subnetPrefixLength = {0};
    QString      _name;
    quint32      _flags = {0};

    friend Interface::List getInterfaces();
};

// Возвращает список сетевых интерфейсов
Interface::List getInterfaces();

} // namespace network
