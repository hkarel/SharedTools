/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2020 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "config/appl_conf.h"
#include "break_point.h"
#include "safe_singleton.h"

#include <unistd.h>
#include <sys/stat.h>

namespace config {

string dir()
{
#if defined(CONFIG_DIR)
    string cfgDir {CONFIG_DIR};
    dirExpansion(cfgDir);
    return cfgDir;
#else
    std::cerr << "Undefined CONFIG_DIR";
    abort();
#endif
}

string path(const string& configFile)
{
    return dir() + "/" + configFile;
}

#ifdef QT_CORE_LIB
QString qdir()
{
    return QString::fromStdString(dir());
}

QString qpath(const QString& configFile)
{
    return qdir() + "/" + configFile;
}
#endif

YamlConfig& base()
{
    return ::safe_singleton<YamlConfig, 0>();
}

YamlConfig& state()
{
    return ::safe_singleton<YamlConfig, 1>();
}

void dirExpansion(std::string& filePath)
{
    if (filePath.empty())
        return;

    auto slashReplace = [&]()
    {
        for (char& c : filePath)
            if (c == '\\') c = '/';
    };

    const char* programData = "ProgramData";
    size_t n = filePath.find(programData);
    if (n == 0)
    {
        const char* prdata = getenv("PROGRAMDATA");
        filePath.replace(0, strlen(programData), string(prdata));
        slashReplace();
        return;
    }

    const char* appData = "AppData";
    n = filePath.find(appData);
    if (n == 0)
    {
        const char* appdata = getenv("APPDATA");
        filePath.replace(0, strlen(appData), string(appdata));
        slashReplace();
        return;
    }

    if (filePath[0] == '~')
    {
        const char* home = getenv("HOME");
        filePath.replace(0, 1, string(home));
        slashReplace();
    }
}

#ifdef QT_CORE_LIB
void dirExpansion(QString& filePath)
{
    string s = filePath.toStdString();
    dirExpansion(s);
    filePath = QString::fromStdString(s);
}
#endif

std::time_t baseModifyTime()
{
    struct stat st;
    ::stat(base().filePath().c_str(), &st);
    return st.st_mtime;
}

std::time_t loggerModifyTime()
{
    string logConf;
    std::time_t confTime = 0;
#ifdef MINGW
    base().getValue("logger.conf_win", logConf);
#else
    base().getValue("logger.conf", logConf);
#endif
    if (!logConf.empty())
    {
        dirExpansion(logConf);
        if (::access(logConf.c_str(), F_OK) != -1)
        {
            struct stat st;
            ::stat(logConf.c_str(), &st);
            confTime = st.st_mtime;
        }
    }
    return confTime;
}

#ifdef QT_NETWORK_LIB
bool readHostAddress(const QString& confHostStr, QHostAddress& hostAddress)
{
    QString hostAddressStr;
    if (!config::base().getValue(confHostStr.toStdString(), hostAddressStr))
        return false;

    hostAddressStr = hostAddressStr.toLower().trimmed();

    if (hostAddressStr == "localhost")    hostAddress = QHostAddress::LocalHost;
    else if (hostAddressStr == "any")     hostAddress = QHostAddress::Any;
#if QT_VERSION >= 0x050000
    else if (hostAddressStr == "any_ip4") hostAddress = QHostAddress::AnyIPv4;
    else if (hostAddressStr == "any_ip6") hostAddress = QHostAddress::AnyIPv6;
#else
    else if (hostAddressStr == "any_ip4") hostAddress = QHostAddress::Any;
    else if (hostAddressStr == "any_ip6") hostAddress = QHostAddress::Any;
#endif
    else                                  hostAddress = QHostAddress(hostAddressStr);

    return true;
}
#endif

} // namespace config
