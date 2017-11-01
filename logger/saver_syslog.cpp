/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2013 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "saver_syslog.h"
#include <syslog.h>

namespace alog {

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
