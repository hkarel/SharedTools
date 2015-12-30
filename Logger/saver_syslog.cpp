/* clang-format off */
#include "saver_syslog.h"
#include <syslog.h>

namespace alog
{

SaverSyslog::SaverSyslog(const char* ident, Level level)
    : Saver("syslog", level)
{
    openlog(/*"LBcore"*/ ident, LOG_PID, LOG_LOCAL5);
}

void SaverSyslog::flushImpl(const MessageList& messages)
{
    if (messages.size() == 0)
        return;

    auto syslogLevel = [] (Level level) -> int
    {
        switch (level)
        {
            case Level::ERROR   : return LOG_ERR;
            case Level::WARNING : return LOG_WARNING;
            case Level::INFO    : return LOG_NOTICE;
            case Level::VERBOSE : return LOG_INFO;
            case Level::DEBUG   : return LOG_DEBUG;
            case Level::DEBUG2  : return LOG_DEBUG;
            default             : return LOG_ERR;
        }
    };

    for (Message* m : messages)
    {
        if (m->level > level())
            continue;

        string str {m->prefix3};
        str += m->str;

        syslog(syslogLevel(m->level), "%s", str.c_str());
    }
}

} // namespace alog
