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
  ---

  Модуль содержит функции по работе с конфигурационными файлами приложения.

*****************************************************************************/

#pragma once

#include "defmac.h"
#include "yaml_config.h"
#include "safe_singleton.h"

#include <ctime>
#include <string>

#ifdef QT_CORE_LIB
#include <QtCore>
#endif
#ifdef QT_NETWORK_LIB
#include <QHostAddress>
#endif

namespace config {

using namespace std;

// Возвращает путь до директории с файлами конфигурации для проекта
string dir();

// Развертывает имя файла конфигурации до полного пути
string path(const string& configFile);

#ifdef QT_CORE_LIB
QString qdir();
QString qpath(const QString& configFile);
#endif

// Базовые настройки приложения
YamlConfig& base();

// Сохраняемые настройки приложения
YamlConfig& state();

// Выполняет расширение пути для двух случаев:
// 1) Символ '~' до полного пути к домашней директории (заменяет функцию homeDirExpansion);
// 2) В Windows кодовое слово 'ProgramData' до директории с программными данными
void dirExpansion(string& filePath);

#ifdef QT_CORE_LIB
void dirExpansion(QString& filePath);
#endif

// Возвращает время модификации базового конфиг-файла
std::time_t baseModifyTime();

// Возвращает время модификации конфиг-файла для логгера
std::time_t loggerModifyTime();

#ifdef QT_NETWORK_LIB
// Возвращает host-адрес из файла конфигурации
bool readHostAddress(const QString& confHostStr, QHostAddress&);
#endif

#ifdef QT_CORE_LIB
/**
  Проверяет факт модификации базового конфиг-файла и файла конфигурации логгера
  по изменению их временной метки. Если изменение временной метки обнаружено -
  выполняется перечитывание файлов конфигурации, и эмитируется сигнал changed().
  Примечание: Функции init(), start(), stop() должны  вызываться  из  основного
              потока приложения после сознания экземпляра QCoreApplication
*/
class ChangeChecker : public QObject
{
public:
    bool init(int timeout = 15 /*сек*/);
    void start();
    void stop();

signals:
    void changed();

private slots:
    void timeout();

private:
    Q_OBJECT
    ChangeChecker();
    DISABLE_DEFAULT_COPY(ChangeChecker)

    QTimer _timer;
    // Время последней модификации конфиг-файлов
    std::time_t _baseModifyTime = {0};
    std::time_t _loggerModifyTime = {0};

    template<typename T, int> friend T& ::safe_singleton();
};

ChangeChecker& changeChecker();
#endif // QT_CORE_LIB

} // namespace config
