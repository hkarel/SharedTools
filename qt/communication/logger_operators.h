#pragma once

#include "logger/logger.h"
#include "qt/quuidex.h"

namespace communication {

class HostPoint;

/**
  Вспомогательная структура, используется для отправки в лог идентификатора
  команды вместе с именем.
*/
struct CommandNameLog
{
    const QUuidEx& command;
    CommandNameLog(const QUuidEx& command) : command(command) {}
};
} // namespace communication

namespace alog {

Line& operator<< (Line&  line, const communication::HostPoint&);
Line  operator<< (Line&& line, const communication::HostPoint&);
Line& operator<< (Line&  line, const communication::CommandNameLog&);
Line  operator<< (Line&& line, const communication::CommandNameLog&);

} // namespace alog
