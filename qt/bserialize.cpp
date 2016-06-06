#include "bserialize.h"

namespace bserial {

QDataStream& operator>> (QDataStream& s, ByteArray& ba)
{
    ba.clear();
    quint32 len;
    s >> len;
    if (len == 0xffffffff)
        return s;

    ba.resize(len);
    if (s.readRawData(ba.data(), len) != int(len))
    {
        ba.clear();
        s.setStatus(QDataStream::ReadPastEnd);
    }
    return s;
}

} // namespace bserial
