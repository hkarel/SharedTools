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
*****************************************************************************/

#include "settings.h"

#include "spin_locker.h"
#include "utils.h"
#include <fstream>

#define log_error_m   alog::logger().error   (alog_line_location, "Settings")
#define log_warn_m    alog::logger().warn    (alog_line_location, "Settings")
#define log_info_m    alog::logger().info    (alog_line_location, "Settings")
#define log_verbose_m alog::logger().verbose (alog_line_location, "Settings")
#define log_debug_m   alog::logger().debug   (alog_line_location, "Settings")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "Settings")


void settingsWrite(const Settings& s, const std::string& fileName)
{
    if (!fileName.empty())
    {
        if (FILE * f = fopen(fileName.c_str(), "w"))
        {
            Settings::NameValList nv = s.values();
            for (auto x : nv)
            {
                std::string str = x.first + " = " + x.second + "\n";
                fputs(str.c_str(), f);
            }
            fclose(f);
        }
        else
            log_error_m << "Can not open file " << fileName << " for writing.";
    }
}

bool Settings::read(const std::string& filePath)
{
    if (filePath.empty())
    {
        log_error_m << "Config file name is empty";
        return false;
    }

    std::ifstream file(filePath, std::ifstream::in);
    if (!file.is_open())
    {
        log_error_m << "Can not open config file: " << filePath;
        return false;
    }

    NameValList nv;
    while (true)
    {
        if (file.eof())
            break;

        if (!file.good())
        {
            log_error_m << "Can not read from config file: " << filePath;
            return false;
        }

        std::string str;
        std::getline(file, str);

        if (str.empty() || str[0] == '#')
            continue;

        std::size_t pos = str.find_first_of("=");
        if (pos != std::string::npos)
        {
            std::string name = str.substr(0, pos);
            std::string value = str.substr(pos + 1);
            utl::trim(name);
            utl::trim(value);
            nv.push_back(make_pair(name, value));
        }
    }

    { //For SpinLocker
        SpinLocker locker(_settingsLock); (void) locker;
        if (&_filePath != &filePath) _filePath = filePath;
    }

    setValues(nv);
    print();
    return true;
}

bool Settings::reRead()
{
    return read(_filePath);
}

std::string Settings::filePath() const
{
    SpinLocker locker(_settingsLock); (void) locker;
    return _filePath;
}

std::vector<std::string> Settings::names() const
{
    SpinLocker locker(_settingsLock); (void) locker;
    std::vector<std::string> v;
    for (const auto& x : _settings) v.push_back(x.first);
    return v;
}

std::string Settings::value(const std::string& name,
                            const std::string& defaultVal,
                            bool logWarnings) const
{
    std::string value;
    if (!exists(name, &value))
    {
        if (logWarnings)
            SETTING_VALUE_NOT_FOUND("Settings")
        return defaultVal;
    }
    return value;
}

Settings::NameValList Settings::values() const
{
    NameValList nv;
    SpinLocker locker(_settingsLock); (void) locker;
    for (const auto& x : _settings)
        nv.push_back(std::make_pair(x.first, x.second));
    return nv;
}

void Settings::addValues(const NameValList& nv)
{
    if (nv.empty())
        return;

    { //Block for SpinLocker
        SpinLocker locker(_settingsLock); (void) locker;
        for (const auto& x : nv) _settings[x.first] = x.second;
    }
    changeSignal.emit_();
}

void Settings::setValues(const NameValList& nv)
{
    if (nv.empty())
        return;

    { //Block for SpinLocker
        SpinLocker locker(_settingsLock); (void) locker;
        _settings.clear();
        for (const auto& x : nv) _settings[x.first] = x.second;
    }
    changeSignal.emit_();
}

bool Settings::empty() const
{
    SpinLocker locker(_settingsLock); (void) locker;
    return _settings.empty();
}

void Settings::clear()
{
    {
        SpinLocker locker(_settingsLock); (void) locker;
        _settings.clear();
    }
    changeSignal.emit_();
}

bool Settings::exists(const std::string& name, std::string* value) const
{
    SpinLocker locker(_settingsLock); (void) locker;
    auto it = _settings.find(name);
    if (it != _settings.end())
    {
        if (value)
            *value = it->second;
        return true;
    }
    return false;
}

void Settings::print()
{
    if (alog::logger().level() >= alog::Level::DEBUG)
    {
        NameValList nv = values();
        alog::Line log_line = log_debug_m << "Read: ";
        for (const auto& x : nv)
            log_line << x.first << " : " << x.second + "; ";
    }
}
