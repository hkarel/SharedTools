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

#pragma once

#include "config/yaml_config.h"
#include <QtCore>
#ifdef QT_NETWORK_LIB
#include <QHostAddress>
#endif

namespace config {

// Возвращает путь до директории с файлами конфигурации для проекта
QString dir();

// Развертывает имя файла конфигурации до полного пути
QString path(const QString& configFile);

// Базовые настройки приложения
YamlConfig& base();

// Сохраняемые настройки приложения
YamlConfig& state();

// Расширяет правую часть пути до файла полным путем до файла относительно
// текущего приложения
QString getFilePath(const QString& partFilePath);

// Расширяет правую часть пути до директории полным путем до директории
// относительно текущего приложения
QString getDirPath(const QString& partDirPath);

// Расширяет символ '~' до полного пути к домашней директории
void homeDirExpansion(QString& filePath);

// Выполняет расширение пути для двух случаев:
// 1) Символ '~' до полного пути к домашней директории (заменяет функцию homeDirExpansion);
// 2) В Windows кодовое слово 'ProgramData' до директории с программными данными
void dirExpansion(QString& filePath);

#ifdef QT_NETWORK_LIB
// Возвращает host-адрес из файла конфигурации
bool readHostAddress(const QString& confHostStr, QHostAddress&);
#endif

} // namespace config
