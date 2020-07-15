#include "logger_config.h"
#include "logger/config.h"
#include "qt/config.h"
#include <QtCore>

namespace alog {

bool configDefaultSaver()
{
    QString logFile;
#ifdef MINGW
    config::base().getValue("logger.file_win", logFile);
    config::dirExpansion(logFile);
#else
    config::base().getValue("logger.file", logFile);
#endif
    config::dirExpansion(logFile);
    QFileInfo logFileInfo {logFile};
    QString logFileDir = logFileInfo.absolutePath();
    if (!QDir(logFileDir).exists())
        if (!QDir().mkpath(logFileDir))
        {
            log_error << "Failed create log directory: " << logFileDir;
            return false;
        }

    // Создаем дефолтный сэйвер для логгера
    std::string logLevelStr = "info";
    config::base().getValue("logger.level", logLevelStr);

    bool logContinue = true;
    config::base().getValue("logger.continue", logContinue);

    Level logLevel = levelFromString(logLevelStr);
    SaverPtr saver {new SaverFile("default",
                                  logFile.toStdString(),
                                  logLevel,
                                  logContinue)};

    // Загружаем фильтры для дефолтного сэйвера
    const YAML::Node filtersNode = config::base().getNode("logger.filters");
    FilterList filters;
    loadFilters(filtersNode, filters);
    for (Filter* filter : filters)
        saver->addFilter(FilterPtr(filter));

    logger().addSaver(saver);
    return true;
}

void configExtensionSavers()
{
    QString logConf;
#ifdef MINGW
    config::base().getValue("logger.conf_win", logConf);
#else
    config::base().getValue("logger.conf", logConf);
#endif
    if (!logConf.isEmpty())
    {
        config::dirExpansion(logConf);
        if (QFile::exists(logConf))
            loadSavers(logConf.toStdString());
        else
            log_error << "Logger config file not exists: " << logConf;
    }
}

} // namespace alog
