#include "settings.h"
#include "spin_locker.h"
#include "utils.h"

#include <fstream>

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Settings")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Settings")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Settings")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "Settings")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Settings")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "Settings")


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
        std::string str;
        std::getline(file, str);

        if (file.eof())
            break;

        if (!file.good())
        {
            log_error_m << "Can not read from config file: " << filePath;
            return false;
        }

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

    if (alog::logger().level() >= alog::Level::DEBUG)
    {
        std::string s = "Read: ";
        for (const auto& x : nv)
            s += x.first + " : " + x.second + "; ";
        log_debug_m << s;
    }

    { //For SpinLocker
        SpinLocker locker(_settingsLock); (void) locker;
        if (&_filePath != &filePath) _filePath = filePath;
    }

    setValues(nv);
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


#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
