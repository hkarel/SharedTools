#pragma once
#include <QByteArray>

namespace qppmd {

int compress(const QByteArray& in, QByteArray& out, int compressionLevel = -1);
int decompress(const QByteArray& in, QByteArray& out);

} // namespace qppmd
