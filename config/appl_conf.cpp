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
#include "config/logger_conf.h"
#include "break_point.h"
#include "safe_singleton.h"
#include "logger/config.h"

#ifdef QT_CORE_LIB
#include "qt/logger_operators.h"
#endif

#include <unistd.h>
#include <sys/stat.h>

#define log_error_m   alog::logger().error   (alog_line_location, "ApplConfig")
#define log_warn_m    alog::logger().warn    (alog_line_location, "ApplConfig")
#define log_info_m    alog::logger().info    (alog_line_location, "ApplConfig")
#define log_verbose_m alog::logger().verbose (alog_line_location, "ApplConfig")
#define log_debug_m   alog::logger().debug   (alog_line_location, "ApplConfig")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "ApplConfig")

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
    return safe::singleton<YamlConfig, 0>();
}

YamlConfig& state()
{
    return safe::singleton<YamlConfig, 1>();
}

YamlConfig& work()
{
    return safe::singleton<YamlConfig, 2>();
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

time_t baseModifyTime()
{
    struct stat st;
    ::stat(base().filePath().c_str(), &st);
    return st.st_mtime;
}

string loggerConfFile()
{
    string logConf;
#ifdef MINGW
    base().getValue("logger.conf_win", logConf);
#else
    base().getValue("logger.conf", logConf);
#endif
    if (!logConf.empty())
    {
        dirExpansion(logConf);
        if (::access(logConf.c_str(), F_OK) != -1)
            return logConf;
        else
            log_error_m << "Logger config file not exists: " << logConf;
    }
    return {};
}

time_t loggerModifyTime()
{
    string logConf = loggerConfFile();
    if (logConf.empty())
        return 0;

    struct stat st;
    ::stat(logConf.c_str(), &st);
    return st.st_mtime;
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
#endif // QT_NETWORK_LIB

#ifdef QT_CORE_LIB
Observer::Observer()
{
    chk_connect_q(&_timer, &QTimer::timeout, this, &Observer::timeout)
}

time_t Observer::modifyTime(const QString& filePath)
{
    struct stat st;
    if (::stat(filePath.toUtf8().constData(), &st) == -1)
        return 0;
    return st.st_mtime;
}

void Observer::start(int timeout)
{
    QMutexLocker locker {&_filesLock}; (void) locker;

    for (auto& p : _files)
        p.first = modifyTime(p.second);

    _timer.setInterval(timeout * 1000);
    _timer.start();
}

void Observer::stop()
{
    _timer.stop();
}

void Observer::addFile(const QString& filePath)
{
    QMutexLocker locker {&_filesLock}; (void) locker;

    bool found = false;
    for (const auto& p : _files)
        if (p.second == filePath)
        {
            found = true;
            break;
        }

    if (!found)
    {
        _files.append({modifyTime(filePath), filePath});
        log_debug_m << "The file added to observer: " << filePath;
    }
}

void Observer::removeFile(const QString& filePath)
{
    QMutexLocker locker {&_filesLock}; (void) locker;

    for (int i = 0; i < _files.count(); ++i)
        if (_files[i].second == filePath)
        {
            _files.removeAt(i--);
            log_debug_m << "The file removed from observer: " << filePath;
        }
}

QStringList Observer::files() const
{
    QMutexLocker locker {&_filesLock}; (void) locker;

    QStringList files;
    for (const auto& p : _files)
        files.append(p.second);

    return files;
}

void Observer::clear()
{
    QMutexLocker locker {&_filesLock}; (void) locker;

    for (const auto& p : _files)
        log_debug_m << "The file removed from observer: " << p.second;
    _files.clear();
}

void Observer::timeout()
{
    QStringList files;
    { //Block for QMutexLocker
        QMutexLocker locker {&_filesLock}; (void) locker;

        for (auto& p : _files)
        {
            if (::access(p.second.toUtf8().constData(), F_OK) == -1)
            {
                log_error_m << "Observer, file not exists: " << p.second;
                continue;
            }
            time_t modifyTime = Observer::modifyTime(p.second);
            if (p.first != modifyTime)
            {
                p.first = modifyTime;
                files.append(p.second);
            }
        }
    }
    for (const QString& f : files)
    {
        log_verbose_m << "Observer, file changed: " << f;
        emit changedItem(f);
    }
    if (files.count())
        emit changed();
}

Observer& observer()
{
    return safe::singleton<Observer>();
}

ObserverBase::ObserverBase()
{
    chk_connect_a(&_observer, &Observer::changedItem,
                  this, &ObserverBase::changedItem)
}

void ObserverBase::start(int timeout)
{
    _observer.clear();
    _observer.addFile(base().filePath().c_str());

    string logConf = loggerConfFile();
    if (!logConf.empty())
        _observer.addFile(logConf.c_str());

    _observer.start(timeout);
}

void ObserverBase::stop()
{
    _observer.stop();
}

void ObserverBase::changedItem(const QString& filePath)
{
    bool modify = false;
    if (filePath == base().filePath().c_str())
    {
        modify = true;
        base().rereadFile();
        log_verbose_m << "Config file reread: " << filePath;
        alog::configDefaultSaver();
    }

    string logConf = loggerConfFile();
    if (!logConf.empty() && (filePath == logConf.c_str()))
    {
        modify = true;
        log_verbose_m << "Logger config file reread: " << filePath;
        alog::configExtendedSavers();
    }

    if (modify)
    {
        alog::printSaversInfo();
        emit changed();
    }
}

ObserverBase& observerBase()
{
    return safe::singleton<ObserverBase>();
}
#endif // QT_CORE_LIB

} // namespace config
