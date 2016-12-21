#pragma once

#include <QObject>
#include <QByteArray>
#include <QMutex>

namespace qlzma {

int compress  (const QByteArray& in, QByteArray& out, int compressionLevel = -1);
int compress2 (const QByteArray& in, QByteArray& out, int compressionLevel = -1);
int decompress(const QByteArray& in, QByteArray& out);

void removeRancidEncoders();

} // namespace qlzma
