#include <cstdio>
#include <cstring>
#include <unistd.h>

//#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>

#include "common/Logger/logger.h"


#include "utils.h"

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

bool mkDir(const string& dir, bool throwExcept)
{
    try
    {
        boost::filesystem::create_directories(dir);
//        if (!boost::filesystem::is_directory(data_dir))
//        {
//            log_error << "Can not create directory " << data_dir;
//            logger().stop();
//            return -1;
//        }
        return true;
    }
    catch (boost::system::system_error& e)
    {
        log_error << "Can not create directory: " << dir << "; Detail: " << e.what();
        if (throwExcept) throw;
    }
    catch (...)
    {
        log_error << "Can not create directory " << dir << "; Unknown error.";
        if (throwExcept) throw;
    }
    return false;
}

} //namespace lbutl



