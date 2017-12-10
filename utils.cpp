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

#include "utils.h"
#include "break_point.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace utl {

string& ltrim(string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

string& rtrim(string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

string& trim(string& s)
{
    return ltrim(rtrim(s));
}

bool setCurrentDir(char* binaryPath)
{
    bool ret = false;
    char  * x;
    if ((x = strrchr(binaryPath, '/')))
    {
        *x = '\0';
        ret = (chdir(binaryPath) == 0);
        *x = '/';
    }
    return ret;
}

void savePidFile(const string& fileName)
{
    if (!fileName.empty())
    {
        if (FILE * pf = fopen(fileName.c_str(), "w"))
        {
            fprintf(pf, "%d\n", getpid());
            fclose(pf);
        }
        else
            throw logic_error("Can not open pid-file " + fileName + " for writing");
    }
}

string toString(short val)
{
    return toString(int(val));
}

string toString(unsigned short val)
{
    return toString(int(val));
}

string toString(int val)
{
    char buff[34] = {0};
    snprintf(buff, sizeof(buff)-1, "%d", val);
    return buff;
}

string toString(unsigned int val)
{
    char buff[34] = {0};
    snprintf(buff, sizeof(buff)-1, "%u", val);
    return buff;
}

string toString(long val)
{
    char buff[68] = {0};
    snprintf(buff, sizeof(buff)-1, "%ld", val);
    return buff;
}

string toString(unsigned long val)
{
    char buff[68] = {0};
    snprintf(buff, sizeof(buff)-1, "%lu", val);
    return buff;
}

string toString(long long val)
{
    char buff[68] = {0};
#if defined(_MSC_VER) || defined(__MINGW32__)
    const char* format = "%I64d";
#else
    const char* format = "%lld";
#endif
    snprintf(buff, sizeof(buff)-1, format, val);
    return buff;
}

string toString(unsigned long long val)
{
    char buff[68] = {0};
#if defined(_MSC_VER) || defined(__MINGW32__)
    const char* format = "%I64u";
#else
    const char* format = "%llu";
#endif
    snprintf(buff, sizeof(buff)-1, format, val);
    return buff;
}

static inline uint8_t toHexChar(uint8_t c) noexcept
{
    return (c < 10) ? uint8_t('0' + c) : uint8_t('a' + (c - 10));
}

string uuidToString(const uint8_t uuid[])
{
//    --- Старая реализация ---
//    string buff;
//    buff.reserve(36);
//    for (int i = 0; i < 16; ++i)
//    {
//        buff += toHexChar((uuid[i] >> 4) & 0x0f);
//        buff += toHexChar(uuid[i] & 0x0f);
//        if (i == 3 || i == 5 || i == 7 || i == 9)
//            buff += '-';
//    }
//    return std::move(buff);

    // Отладить
    break_point

    uint8_t buff[40];
    uuidToString(uuid, buff);
    return std::string((const char*)buff);
}

void uuidToString(const uint8_t uuid[], uint8_t result[])
{
    uint8_t* r = result;
    for (int i = 0; i < 16; ++i)
    {
        (*r++) = toHexChar((uuid[i] >> 4) & 0x0f);
        (*r++) = toHexChar(uuid[i] & 0x0f);
        if (i == 3 || i == 5 || i == 7 || i == 9)
            (*r++) = '-';
    }
    *r = '\0';
}

string uuidToHexString(const uint8_t uuid[], bool addHexPrefix)
{
//    --- Старая реализация ---
//    string buff;
//    if (addHexPrefix)
//    {
//        buff.reserve(34);
//        buff += "0x";
//    }
//    else
//        buff.reserve(32);

//    for (int i = 0; i < 16; ++i)
//    {
//        buff += toHexChar((uuid[i] >> 4) & 0x0f);
//        buff += toHexChar(uuid[i] & 0x0f);
//    }
//    return std::move(buff);

    // Отладить
    break_point

    uint8_t buff[40];
    uuidToHexString(uuid, buff, addHexPrefix);
    return std::string((const char*)buff);
}

void uuidToHexString(const uint8_t uuid[], uint8_t result[], bool addHexPrefix)
{
    uint8_t* r = result;
    if (addHexPrefix)
    {
        (*r++) = '0';
        (*r++) = 'x';
    }
    for (int i = 0; i < 16; ++i)
    {
        (*r++) = toHexChar((uuid[i] >> 4) & 0x0f);
        (*r++) = toHexChar(uuid[i] & 0x0f);
    }
    *r = '\0';
}

vector<string> split(const string& str, char delim, bool keepEmptyParts)
{
    stringstream stream(str);
    string item;
    vector<string> elems;
    while (std::getline(stream, item, delim))
    {
        if (item.empty() && !keepEmptyParts)
            continue;
        elems.push_back(item);
        item.clear();
    }
    return std::move(elems);
}

double round(double number, int signCount)
{
    int n = 1;
    switch (signCount)
    {
        case 0: n = 1;      break;
        case 1: n = 10;     break;
        case 2: n = 100;    break;
        case 3: n = 1000;   break;
        case 4: n = 10000;  break;
        case 5: n = 100000; break;
        default:
            for (int i = 0; i < signCount; ++i)
                n *= 10;
    }
    return std::round(number * n) / n;
}

void timeAdd(const timeval& a, const timeval& b, timeval& result)
{
    result.tv_sec = a.tv_sec + b.tv_sec;
    result.tv_usec = a.tv_usec + b.tv_usec;
    if (result.tv_usec >= 1000000)
    {
        ++result.tv_sec;
        result.tv_usec -= 1000000;
    }
}

void timeSub(const timeval& a, const timeval& b, timeval& result)
{
    result.tv_sec = a.tv_sec - b.tv_sec;
    result.tv_usec = a.tv_usec - b.tv_usec;
    if (result.tv_usec < 0)
    {
        --result.tv_sec;
        result.tv_usec += 1000000;
    }
}

} // namespace utl
