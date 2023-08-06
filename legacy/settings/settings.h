/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2013 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  В модуле реализован простой механизм чтения конфигурационных файлов. Наст-
  ройки должны быть записаны в виде списка пар: параметр = значение. В классе
  Settings реализованы методы по считыванию (и перечитыванию) настроек из
  файла, а так же функция сохранения данных в файл. Если строка начинается
  с символа '#' - она считается комментарием и игнорируется.
*****************************************************************************/

#pragma once

#include "safe_singleton.h"
#include "simple_signal.h"
#include "logger/logger.h"

#include <iostream>
#include <sstream>
#include <atomic>
#include <vector>
#include <map>
#include <cxxabi.h>


class Settings
{
public:
    typedef std::vector<std::pair<std::string, std::string>> NameValList;

    Settings() {}

    // Читает файл конфигурации. filePath - полное имя файла.
    bool read(const std::string& filePath);

    // Перечитывает файл конфигурации
    bool reRead();

    // Возвращает список параметров
    std::vector<std::string> names() const;

    // Возвращает значение параметра по имени
    std::string value(const std::string& name,
                      const std::string& defaultVal = "",
                      bool logWarnings = true) const;

    // Возвращает список пар (имя:значение)
    NameValList values() const;

    template<typename T>
    void setValue(const std::string& name, const T& value);

    // Добавляет новые параметры к уже существующим. Если наименования параметров
    // совпадают, то старые параметры будут заменены новыми.
    void addValues(const NameValList&);

    // Устанавливает новые параметры из списка NameValList, при этом все старые
    // параметры будут удалены.
    void setValues(const NameValList&);

    template<typename T>
    T valueTo(const std::string& name,
              const T& defaultVal = T(),
              bool logWarnings = true) const;

    // Возвращает полное имя файла конфигурации
    std::string filePath() const;

    // Возврашает TRUE если список элементов пуст
    bool empty() const;

    // Очищает список элементов
    void clear();

    // Возвращает TRUE если параметр name есть в списке, если value определен,
    // то в него будет записано значение параметра.
    bool exists(const std::string& name, std::string* value = 0) const;

    // Эмитирует событие изменения данных
    SimpleSignal<void ()> changeSignal;

    // Использовать abi_type_name()
    // template<typename T>
    // static char* typeName();

    // Выводит список настроек в систему логирования
    void print();


private:
    Settings(Settings&&) = delete;
    Settings(const Settings&) = delete;
    Settings& operator= (Settings&&) = delete;
    Settings& operator= (const Settings&) = delete;

    std::string _filePath;
    std::map<std::string, std::string> _settings;
    mutable std::atomic_flag  _settingsLock = ATOMIC_FLAG_INIT;
};

// Реализация функции settings() должна быть выполнена в модуле main.cpp, это
// требование связано с особенностью сборки и работы so-модулей проекта LBcore.
// Если это требование не выполнять, то каждый so-модуль в LBcore будет иметь
// собственный экземпляр settings().
Settings& settings(); // {return safe_singleton<Settings>();}

// Используется для записи настроек в отдельный файл
void settingsWrite(const Settings&, const std::string& fileName);


//----------------------- Implementation Settings ---------------------------

template<typename T>
void Settings::setValue(const std::string& name, const T& value)
{
    { //Block for SpinLocker
        SpinLocker locker(_settingsLock); (void) locker;
        std::stringstream s;
        s << value;
        _settings[name] = s.str();
    }
    changeSignal.emit_();
}

//template<typename T>
//char* Settings::typeName()
//{
//#if defined(_MSC_VER)
//    return typeid(T).name();
//#else
//    return abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
//#endif
//}

#define SETTING_VALUE_NOT_FOUND(MODULE_NAME) \
    alog::logger().warn(alog_line_location, MODULE_NAME) \
        << "Config parameter " << name << " not found. " \
        << "Will be returned default value: \"" << defaultVal << "\"";

#define SETTING_VALUE_FAILED_CONVERT(MODULE_NAME) \
    alog::logger().warn(alog_line_location, MODULE_NAME) \
        << "Failed to convert parameter " << name << " = '" << value << "' " \
        << "to type '" << Settings::typeName<T>() << "'. " \
        << "Will be returned default value: \"" << defaultVal << "\"";

template<typename T>
T Settings::valueTo(const std::string& name,
                    const T& defaultVal,
                    bool logWarnings) const
{
    std::string value;
    if (!exists(name, &value))
    {
        if (logWarnings)
            SETTING_VALUE_NOT_FOUND("Settings")
        return defaultVal;
    }

    std::stringstream s(value);
    T t; s >> t;
    if (s.fail())
    {
        if (logWarnings)
            SETTING_VALUE_FAILED_CONVERT("Settings")
        return defaultVal;
    }
    return t;
}
