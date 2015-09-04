#include "utils.h"
#include "Logger/logger.h"

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <algorithm>

namespace utl
{

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

string toString(int val)
{
    char buff[34] = {0};
    snprintf(buff, sizeof(buff)-1, "%d", val);
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
    return buff;
}

string uuidToHexString(const uint8_t* uuid)
{
    string buff;
    buff.reserve(34);
    buff += "0x";
    for (int i = 0; i < 16; ++i)
    {
        buff += toHexChar((uuid[i] >> 4) & 0x0f);
        buff += toHexChar(uuid[i] & 0x0f);
    }
    return buff;
}

} // namespace utl
