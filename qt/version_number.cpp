/*****************************************************************************
  The MIT License

  Copyright © 2015 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "version_number.h"

VersionNumber::VersionNumber(quint8 major, quint8 minor, quint8 patch)
{
    ver.major = major;
    ver.minor = minor;
    ver.patch = patch;
    ver.build = 0;
}

QString VersionNumber::toString() const
{
    return QString("%1.%2.%3").arg(ver.major).arg(ver.minor).arg(ver.patch);
}

bool operator== (const VersionNumber v1, const VersionNumber v2)
{
    return (v1.vers == v2.vers);
}

bool operator!= (const VersionNumber v1, const VersionNumber v2)
{
    return (v1.vers != v2.vers);
}

static int compare(const VersionNumber v1, const VersionNumber v2)
{
    #define COMPARE(f1, f2) if (f1 != f2) return (f1 < f2) ? -1 : 1;
    COMPARE(v1.ver.major, v2.ver.major)
    COMPARE(v1.ver.minor, v2.ver.minor)
    COMPARE(v1.ver.patch, v2.ver.patch)
    #undef COMPARE
    return 0;
}

bool operator> (const VersionNumber v1, const VersionNumber v2)
{
    return (compare(v1, v2) == 1);
}

bool operator< (const VersionNumber v1, const VersionNumber v2)
{
    return (compare(v1, v2) == -1);
}

bool operator>= (const VersionNumber v1, const VersionNumber v2)
{
    return (v1 > v2) || (v1 == v2);
}

bool operator<= (const VersionNumber v1, const VersionNumber v2)
{
    return (v1 < v2) || (v1 == v2);
}

const VersionNumber& productVersion()
{
    const static VersionNumber version(VERSION_PROJECT_MAJOR,
                                       VERSION_PROJECT_MINOR,
                                       VERSION_PROJECT_PATCH);
    return version;
}
