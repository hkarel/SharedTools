#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <algorithm>

#include "Logger/logger.h"
#include "utils.h"


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

string uuidToHexString(const uint8_t* uuid)
{
    auto to_hex_char = [](uint8_t c) -> uint8_t
    {
        if (c <= 9)
            return '0' + c;
        return 'a' + (c - 10);
    };

    string buff;
    buff.reserve(35);
    buff += "0x";
    for (int i = 0; i < 16; ++i)
    {
        buff += to_hex_char((uuid[i] >> 4) & 0x0f);
        buff += to_hex_char(uuid[i] & 0x0f);
    }
    return buff;
}


} // namespace utl

