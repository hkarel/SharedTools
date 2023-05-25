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

#include "config/logger_conf.h"
#include "config/appl_conf.h"
#include "logger/logger.h"
#include "logger/config.h"

#include <unistd.h>
#include <sys/stat.h>

#define log_error_m   alog::logger().error   (alog_line_location, "LogConfig")
#define log_warn_m    alog::logger().warn    (alog_line_location, "LogConfig")
#define log_info_m    alog::logger().info    (alog_line_location, "LogConfig")
#define log_verbose_m alog::logger().verbose (alog_line_location, "LogConfig")
#define log_debug_m   alog::logger().debug   (alog_line_location, "LogConfig")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "LogConfig")

namespace alog {

using namespace std;

bool createLoggerParams(const string& loggerFile)
{
    if (config::base().readOnly())
    {
        log_error_m << "Unable to create log-saver parameters for logger"
                    << ". Config data is read only";
        return false;
    }
    if (config::base().saveDisabled())
    {
        log_error_m << "Unable to create log-saver parameters for logger"
                    << ". Save data is disabled for config file";
        return false;
    }

    bool modify = false;

    string logLevelStr;
    if (!config::base().getValue("logger.level", logLevelStr))
    {
        logLevelStr = "info";
        config::base().setValue("logger.level", logLevelStr);
        modify = true;
    }

    bool logContinue;
    if (!config::base().getValue("logger.continue", logContinue))
    {
        config::base().setValue("logger.continue", false);
        modify = true;
    }

    string logFile;
#ifdef MINGW
    if (!config::base().getValue("logger.file_win", logFile))
    {
        config::base().setValue("logger.file_win", loggerFile);
        modify = true;
    }
#else
    if (!config::base().getValue("logger.file", logFile))
    {
        config::base().setValue("logger.file", loggerFile);
        modify = true;
    }
#endif

    bool ret = true;
    if (modify)
        ret = config::base().saveFile();

    return ret;
}

bool configDefaultSaver()
{
    string logFile;
#ifdef MINGW
    config::base().getValue("logger.file_win", logFile);
#else
    config::base().getValue("logger.file", logFile);
#endif
    config::dirExpansion(logFile);

    string logDir = logFile;
    auto begin = std::find_if_not(logDir.rbegin(), logDir.rend(),
                 [](unsigned char c) {return (c != '/');}).base();
    logDir.erase(begin, logDir.end());

    if (logDir.empty())
    {
        log_error_m << "Log directory path not set";
        return false;
    }

    struct stat st;
    ::stat(logDir.c_str(), &st);
    if (!S_ISDIR(st.st_mode))
    {
        log_error_m << "Log directory not exists: " << logDir;
        return false;
    }

    //QFileInfo logFileInfo {logFile};
    //QString logFileDir = logFileInfo.absolutePath();
    //if (!QDir(logFileDir).exists())
    //    if (!QDir().mkpath(logFileDir))
    //    {
    //        log_error << "Failed create log directory: " << logFileDir;
    //        return false;
    //    }

    // Создаем дефолтный сэйвер для логгера
    string logLevelStr = "info";
    config::base().getValue("logger.level", logLevelStr);

    bool logContinue = true;
    config::base().getValue("logger.continue", logContinue);

    Level logLevel = levelFromString(logLevelStr);
    SaverPtr saver {new SaverFile("default", logFile, logLevel, logContinue)};

    int maxLineSize = -1;
    config::base().getValue("logger.max_line_size", maxLineSize, false);
    if (maxLineSize >= 0)
        saver->setMaxLineSize(maxLineSize);

    // Загружаем фильтры для дефолтного сэйвера
    FilterList filters;
    { //Block for locker
        auto locker {config::base().locker()}; (void) locker;
        const YAML::Node filtersNode = config::base().node("logger.filters");
        loadFilters(filtersNode, filters, config::base().filePath());
    }
    for (Filter* filter : filters)
        saver->addFilter(FilterPtr(filter));

    logger().addSaver(saver);
    return true;
}

void configExtendedSavers()
{
    string logConf;
#ifdef MINGW
    config::base().getValue("logger.conf_win", logConf);
#else
    config::base().getValue("logger.conf", logConf);
#endif
    if (!logConf.empty())
    {
        config::dirExpansion(logConf);
        if (::access(logConf.c_str(), F_OK) != -1)
            loadSavers(logConf);
        else
            log_error_m << "Logger config file not exists: " << logConf;
    }
}

} // namespace alog
