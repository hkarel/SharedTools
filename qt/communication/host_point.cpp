#include "qt/communication/host_point.h"
#include <stdexcept>

namespace communication {

DEFINE_B_SERIALIZE_STREAM_OPERATORS

uint qHash(const HostPoint& hp)
{
    return qHash(qMakePair(hp.address(), hp.port()));
}

HostPoint::HostPoint(const QHostAddress& a, int p)
{
    setAddress(a);
    setPort(p);
}

bool HostPoint::operator== (const HostPoint& hp) const
{
    return (_address == hp._address) && (_port == hp._port);
}

bool HostPoint::isNull() const
{
    return _address.isNull() && (_port == 0);
}

void HostPoint::reset()
{
    _address = QHostAddress();
    _port = 0;
}

void HostPoint::setPort(int port)
{
    if (port < 1 || port > 65535)
    {
        QString err = "A port must be in interval 1 - 65535. Assigned value: %1";
        err = err.arg(port);
        throw std::logic_error(std::string(err.toUtf8().constData()));
    }
    _port = quint16(port);
}

bserial::RawVector HostPoint::toRaw() const
{
    B_SERIALIZE_V1(stream)
    stream << _address;
    stream << _port;
    B_SERIALIZE_RETURN
}

void HostPoint::fromRaw(const bserial::RawVector& vect)
{
    B_DESERIALIZE_V1(vect, stream)
    stream >> _address;
    stream >> _port;
    B_DESERIALIZE_END
}

} // namespace communication
