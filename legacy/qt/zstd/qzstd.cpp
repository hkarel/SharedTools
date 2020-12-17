/*****************************************************************************
  The MIT License

  Copyright © 2016 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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
