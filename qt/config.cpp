/*****************************************************************************
  The MIT License

  Copyright © 2015 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "qt/config.h"
#include "qt/logger_operators.h"

#include <stdlib.h>
#include <iostream>

namespace config {

QString dir()
{
#if defined(CONFIG_DIR)
    QString cfgDir {CONFIG_DIR};
    dirExpansion(cfgDir);
    return cfgDir;
#else
    std::cerr << "Undefined CONFIG_DIR";
    abort();
#endif
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

QString getDirPath(const QString& partDirPath)
{
    QDir dir {QCoreApplication::applicationDirPath() + "/../" + partDirPath};
    if (dir.exists())
        return dir.absolutePath();

    QDir dir2 {QCoreApplication::applicationDirPath() + "/../../../" + partDirPath};
    if (dir2.exists())
        return dir2.absolutePath();

    return QString();
}

void dirExpansion(QString& filePath)
{
    if (filePath.isEmpty())
        return;

    const char* programData = "ProgramData";
    if (filePath.startsWith(QLatin1String(programData), Qt::CaseSensitive))
    {
        const char* prdata = getenv("PROGRAMDATA");
        filePath.replace(0, strlen(programData), QString(prdata));
        filePath.replace(QChar('\\'), QChar('/'));
        return;
    }
    if (filePath[0] == QChar('~'))
    {
        filePath.replace(0, 1, QDir::home().path());
        filePath.replace(QChar('\\'), QChar('/'));
    }
}

#ifdef QT_NETWORK_LIB
bool readHostAddress(const QString& confHostStr, QHostAddress& hostAddress)
{
    QString hostAddressStr;
    if (config::base().getValue(confHostStr.toStdString(), hostAddressStr))
    {
        if (hostAddressStr.toLower().trimmed() == "localhost")
            hostAddress = QHostAddress::LocalHost;
        else if (hostAddressStr.toLower().trimmed() == "any")
            hostAddress = QHostAddress::Any;
        else
            hostAddress = QHostAddress(hostAddressStr);

        return true;
    }
    return false;
}
#endif

} // namespace config
