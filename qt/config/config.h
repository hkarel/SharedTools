#pragma once

#include "config/yaml_config.h"
#include <QtCore>

namespace config {

// Возвращает путь до директории с файлами конфигурации для проекта
QString dir();

// Развертывает имя файла конфигурации до полного пути
QString path(const QString& configFile);

// Базовые настройки приложения
YamlConfig& base();

// Сохраняемые настройки приложения
YamlConfig& state();

QString getFilePath(const QString& partFilePath);

// Расширяет символ '~' до полного пути к домашней директории
void homeDirExpansion(QString& filePath);

} // namespace config
