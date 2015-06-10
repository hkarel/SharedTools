#pragma once

#include "logger.h"


namespace lblog
{
using namespace std;

/**
  Вывод в syslog
*/
class SaverSyslog : public Saver
{
public:
    SaverSyslog(const char* ident, Level level = ERROR);
    void flushImpl(const MessageList&) override;
};

} // namespace lblog
