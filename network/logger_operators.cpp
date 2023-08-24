/*****************************************************************************
  The MIT License

  Copyright © 2019 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "network/logger_operators.h"

#if !defined(MINGW) && !defined(_MSC_VER)
#ifndef FREEBSD
#include <netinet/ether.h>
#endif // FREEBSD
#endif // MINGW

namespace alog {

Line& operator<< (Line& line, const sockaddr_in& in)
{
    #define INET_NTOA(IP) int((IP) >> 24), int(((IP) >> 16) & 0xFF), int(((IP) >> 8) & 0xFF), int((IP) & 0xFF)
    if (line.toLogger())
    {
        char buff[50] = {0};
        uint32_t addr = ntohl(in.sin_addr.s_addr);
        snprintf(buff, sizeof(buff)-1, "%d.%d.%d.%d:%d",
                 INET_NTOA(addr), int(ntohs(in.sin_port)));
        line << buff;
    }
    #undef INET_NTOA
    return line;
}

#if !defined(MINGW) && !defined(_MSC_VER)
Line& operator<< (Line& line, const ether_addr& addr)
{
    if (line.toLogger())
    {
        char buff[100] = {0};
        ether_ntoa_r(&addr, buff);
        line << buff;
    }
    return line;
}

#ifndef FREEBSD
Line& operator<< (Line& line, const sockaddr_nl& nl)
{
    if (line.toLogger())
    {
        char buff[100] = {0};
        snprintf(buff, sizeof(buff)-1, "[%u:%u]", nl.nl_pid, nl.nl_groups);
        line << buff;
    }
    return line;
}
#endif // FREEBSD
#endif // MINGW

} // namespace alog
