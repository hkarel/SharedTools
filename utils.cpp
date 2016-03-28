/* clang-format off */
#include "utils.h"
#include "logger/logger.h"

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <algorithm>

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
            log_error << "Can not open pid-file " << fileName << " for writing.";
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
    return (c < 10) ? ('0' + c) : ('a' + (c - 10));
}

string uuidToString(const uint8_t* uuid)
{
    string buff;
    buff.reserve(36);
    for (int i = 0; i < 16; ++i)
    {
        buff += toHexChar((uuid[i] >> 4) & 0x0f);
        buff += toHexChar(uuid[i] & 0x0f);
        if (i == 3 || i == 5 || i == 7 || i == 9)
            buff += '-';
    }
    return std::move(buff);
}

string uuidToHexString(const uint8_t* uuid, bool addHexPrefix)
{
    string buff;
    if (addHexPrefix)
    {
        buff.reserve(34);
        buff += "0x";
    }
    else
        buff.reserve(32);

    for (int i = 0; i < 16; ++i)
    {
        buff += toHexChar((uuid[i] >> 4) & 0x0f);
        buff += toHexChar(uuid[i] & 0x0f);
    }
    return std::move(buff);
}

} // namespace utl
