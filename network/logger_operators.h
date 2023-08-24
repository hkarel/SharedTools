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
  ---

  Модуль содержит специализированные потоковые операторы '<<' для логгера

*****************************************************************************/

#pragma once

#include "defmac.h"
#include "logger/logger.h"
#include <sys/types.h>

#if defined(MINGW) || defined(_MSC_VER)
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <net/ethernet.h>
#ifndef FREEBSD
#include <linux/netlink.h>
#endif // FREEBSD
#endif // MINGW

namespace alog {

Line& operator<< (Line& line, const sockaddr_in&);

#if !defined(MINGW) && !defined(_MSC_VER)
Line& operator<< (Line& line, const ether_addr&);

#ifndef FREEBSD
Line& operator<< (Line& line, const sockaddr_nl&);
#endif // FREEBSD
#endif // MINGW

} // namespace alog
