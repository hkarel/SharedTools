#include "qzstd.h"
#include "zstd.h"

namespace qzstd {

QByteArray compress(const QByteArray& input, int compressionLevel)
{
    if (input.isEmpty())
        return input;

    compressionLevel = qBound(0, compressionLevel, 22);

    QByteArray buff;
    const size_t buffSize = ZSTD_compressBound(input.size());
    buff.resize(buffSize);

    const size_t cSize = ZSTD_compress((void*)buff.constData(), buffSize,
                                       (void*)input.constData(), input.size(),
                                       compressionLevel);
    buff.resize(cSize);
    return buff;
}

QByteArray decompress(const QByteArray& input)
{
    if (input.isEmpty())
        return input;

    QByteArray buff;
    const size_t buffSize =
        ZSTD_getDecompressedSize((void*)input.constData(), input.size());
    if (buffSize == 0)
    {
        qDebug("qzstd::decompress : original size unknown");
        return QByteArray();
    }
    buff.resize(buffSize);

    const size_t dSize = ZSTD_decompress((void*)buff.constData(), buffSize,
                                         (void*)input.constData(), input.size());
    if (buffSize != dSize)
    {
        qDebug("qzstd::decompress : error decoding: %s", ZSTD_getErrorName(dSize));
        return QByteArray();
    }
    return buff;
}

} // namespace qzstd
