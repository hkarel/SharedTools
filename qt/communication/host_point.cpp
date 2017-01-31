#include "qt/communication/host_point.h"

namespace communication {

HostPoint::HostPoint(const QHostAddress& a, quint16 p) : address(a), port(p)
{}

bool HostPoint::operator== (const HostPoint& hp) const
{
    return (address == hp.address) && (port == hp.port);
}

bool HostPoint::isNull() const
{
    return address.isNull() && (port == 0);
}

uint qHash(const HostPoint& hp)
{
    return qHash(qMakePair(hp.address, hp.port));
}

} // namespace communication
