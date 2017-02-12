#include "qt/communication/host_point.h"

namespace communication {

DEFINE_B_SERIALIZE_STREAM_OPERATORS

uint qHash(const HostPoint& hp)
{
    return qHash(qMakePair(hp.address, hp.port));
}

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

void HostPoint::reset()
{
    address = QHostAddress();
    port = 0;
}

bserial::RawVector HostPoint::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << address;
    stream << port;
    B_SERIALIZE_RETURN
}

void HostPoint::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> address;
    stream >> port;
    B_DESERIALIZE_END
}

} // namespace communication
