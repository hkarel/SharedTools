/****************************************************************************
  Модуль содержит специализированные потоковые операторы '<<' для логгера.

****************************************************************************/

#pragma once

#include "logger/logger.h"
#include "qt/quuidex.h"
#include <QtCore>
#include <QHostAddress>

namespace alog {

Line& operator<< (Line&,  const QString&);
Line  operator<< (Line&&, const QString&);
Line& operator<< (Line&,  const QByteArray&);
Line  operator<< (Line&&, const QByteArray&);
Line& operator<< (Line&,  const QUuid&);
Line  operator<< (Line&&, const QUuid&);
Line& operator<< (Line&,  const QUuidEx&);
Line  operator<< (Line&&, const QUuidEx&);
Line& operator<< (Line&,  const QHostAddress&);
Line  operator<< (Line&&, const QHostAddress&);

} // namespace alog
