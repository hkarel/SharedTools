#pragma once

#include <QtCore>

namespace qzstd {

QByteArray compress(const QByteArray& input, int compressionLevel = 3 /*1-22*/);
QByteArray decompress(const QByteArray& input);

} // namespace qzstd



