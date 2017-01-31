#pragma once

#include "qt/bserialize.h"
#include <QtCore>
#include <QHostAddress>

namespace communication {

/**
  Структура группирует адрес и порт
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

    // Функции сериализации данных
    DECLARE_B_SERIALIZE_FUNC
};
uint qHash(const HostPoint&);

} // namespace communication
