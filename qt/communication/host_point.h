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
    HostPoint(const QHostAddress& address, int port);

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
