#pragma once

#include <QtCore>
#include <QHostAddress>

namespace communication {

/**
  С помощью этой структуры определяются "координаты" полученного сообщения,
  а так же задаются "координаты" для доставки сообщения через UDP-сокет.
*/
struct HostPoint
{
    typedef QSet<HostPoint> Set;

    QHostAddress address;
    quint16 port = {0};

    HostPoint() = default;
    HostPoint(const QHostAddress& address, quint16 port);
    bool operator== (const HostPoint&) const;
    bool isNull() const;
};
uint qHash(const HostPoint&);

} // namespace communication
