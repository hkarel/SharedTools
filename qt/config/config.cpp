#include "logger/logger.h"
#include "qt/config/config.h"
#include "qt/logger/logger_operators.h"

#include <exception>

namespace config {

QString dir()
{
    #ifndef CONFIG_DIR
    #error "Undefined CONFIG_DIR"
    #endif

#if defined(__MINGW32__)
    QString cfgDir = QDir::homePath() + "/" + CONFIG_DIR;
#else
    QString cfgDir = "/etc/" CONFIG_DIR;
#endif

    QDir dir;
    if (!dir.exists(cfgDir))
    {
        if (dir.mkpath(cfgDir))
            log_debug << "Has been created config directory: " << cfgDir;
        else
        {
            QString err = "Failed create config directory: " + cfgDir;
            throw std::logic_error(err.toStdString());
        }
    }
    return cfgDir;
}

QString path(const QString& configFile)
{
    return dir() + "/" + configFile;
}

YamlConfig& base()
{
    return ::safe_singleton<YamlConfig, 0>();
}

YamlConfig& state()
{
    return ::safe_singleton<YamlConfig, 1>();
}

QString getFilePath(const QString& partFilePath)
{
    QString filePath = QCoreApplication::applicationDirPath() + "/../" + partFilePath;
    if (!QFile::exists(filePath))
    {
        filePath = QCoreApplication::applicationDirPath() + "/../../../" + partFilePath;
        if (!QFile::exists(filePath))
            return QString();
    }
    return filePath;
}

void homeDirExpansion(QString& filePath)
{
    if (!filePath.isEmpty() && (filePath[0] == QChar('~')))
        filePath.replace(0, 1, QDir::home().path());
}

} // namespace config