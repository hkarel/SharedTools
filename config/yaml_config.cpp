/* clang-format off */
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

#include "yaml_config.h"
#include "break_point.h"
#include "spin_locker.h"
#include "utils.h"

#include <cerrno>
#include <chrono>
#include <fstream>
#include <stdexcept>

#if defined(QT_CORE_LIB)
#include <QByteArray>
#endif

#define log_error_m   alog::logger().error   (alog_line_location, "YamlConfig")
#define log_warn_m    alog::logger().warn    (alog_line_location, "YamlConfig")
#define log_info_m    alog::logger().info    (alog_line_location, "YamlConfig")
#define log_verbose_m alog::logger().verbose (alog_line_location, "YamlConfig")
#define log_debug_m   alog::logger().debug   (alog_line_location, "YamlConfig")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "YamlConfig")

#define YAML_CONFIG_CATCH_2 \
    } catch (YAML::Exception& e) { \
        alog::Line logLine = log_error_m << "YAML error. Detail: " << e.what(); \
        if (!_filePath.empty()) \
            logLine << ". File: " << _filePath; \
        return false; \
    } catch (std::exception& e) { \
        alog::Line logLine = log_error_m << "YAML error. Detail: " << e.what(); \
        if (!_filePath.empty()) \
            logLine << ". File: " << _filePath; \
        return false; \
    } catch (...) { \
        alog::Line logLine = log_error_m << "YAML error. Detail: Unknown error"; \
        if (!_filePath.empty()) \
            logLine << ". File: " << _filePath; \
        return false; \
    }


bool YamlConfig::readFile(const std::string& filePath)
{
    std::lock_guard<std::recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
    _filePath = filePath;
    std::ifstream file(_filePath, std::ifstream::in);
    if (!file.is_open())
    {
        log_warn_m << "Cannot open config file: " << _filePath;
        return false;
    }
    _root = YAML::Load(file);
    if (!_root.IsDefined())
    {
        log_error_m << "Undefined YAML-structure for config file: " << filePath;
        return false;
    }
    YAML_CONFIG_CATCH_2
    _changed = false;
    return true;
}

bool YamlConfig::readString(const std::string& str)
{
    std::lock_guard<std::recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
    _filePath.clear();
    _root = YAML::Load(str);
    if (!_root.IsDefined())
    {
        log_error_m << "Undefined YAML-structure in input string";
        return false;
    }
    YAML_CONFIG_CATCH_2
    _changed = false;
    return true;
}

bool YamlConfig::rereadFile()
{
    return readFile(_filePath);
}

bool YamlConfig::changed() const
{
    return _changed;
}

bool YamlConfig::readOnly() const
{
    return _readOnly;
}

void YamlConfig::setReadOnly(bool val)
{
    _readOnly = val;
}

bool YamlConfig::saveDisabled() const
{
    return _saveDisabled;
}

void YamlConfig::setSaveDisabled(bool val)
{
    _saveDisabled = val;
}

bool YamlConfig::save(const std::string& filePath,
                      YAML::EmitterStyle::value nodeStyle)
{
    if (_saveDisabled)
    {
        log_warn_m << "Save data is disabled. File: " << _filePath;
        return false;
    }

    std::lock_guard<std::recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
    if (!filePath.empty())
        _filePath = filePath;

    if (_filePath.empty())
    {
        log_error_m << "Cannot save data. Undefined file path";
        return false;
    }

    typedef std::chrono::high_resolution_clock clock;
    uint64_t timeTick = clock::now().time_since_epoch().count();
    std::string fileTmp = _filePath + ".tmp" + utl::toString(timeTick);
    std::remove(fileTmp.c_str());

    try
    {
        std::ofstream file {fileTmp, std::ios_base::out};
        if (!file.is_open())
        {
            log_error_m << "Cannot open temporary file " << fileTmp << " for write";
            std::remove(fileTmp.c_str());
            return false;
        }

        _root.SetStyle(nodeStyle);
        file << _root;
        file.flush();
        file.close();
    }
    catch (...)
    {
        std::remove(fileTmp.c_str());
        throw;
    }

    if (0 != std::remove(_filePath.c_str()))
        if (errno != ENOENT)
        {
            log_error_m << "Failed remove old data file " << _filePath;
            std::remove(fileTmp.c_str());
            return false;
        }

    if (0 != std::rename(fileTmp.c_str(), _filePath.c_str()))
    {
        log_error_m << "Failed rename temporary file " << fileTmp
                    << " to " << _filePath;
        return false;
    }
    YAML_CONFIG_CATCH_2
    _changed = false;
    return true;
}

std::string YamlConfig::filePath() const
{
    return _filePath;
}

bool YamlConfig::remove(const std::string& name, bool logWarn)
{
    std::lock_guard<std::recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
     size_t pos = name.find_last_of('.');
     if (pos == std::string::npos)
     {
         _root.remove(name);
         return true;
     }
     std::string namePre = name.substr(0, pos);
     std::string nameKey = name.substr(pos + 1);

     YAML::Node node = getNode(namePre);
     if (!node || node.IsNull())
         return false;

    node.remove(nameKey);
    YAML_CONFIG_CATCH_2
    return true;
}

YAML::Node YamlConfig::getNode(const std::string& name) const
{
    return getNode(_root, name);
}

YAML::Node YamlConfig::getNode(const YAML::Node& baseNode,
                               const std::string& name) const
{
    if (name == ".")
        return baseNode;

    std::vector<std::string> parts = utl::split(name, '.');
    if (parts.empty())
        return YAML::Node();

    std::lock_guard<std::recursive_mutex> locker {_configLock}; (void) locker;

    typedef std::function<YAML::Node (const YAML::Node&, size_t)> NodeFunc;
    NodeFunc get_node = [&](const YAML::Node& node, size_t i)
    {
        if (!node.IsDefined() || node.IsNull())
            return YAML::Node();

        if (i == parts.size())
            return node;

        if (!node.IsMap())
            return YAML::Node();

        YAML_CONFIG_TRY
        std::string s = parts[i];
        YAML::Node n = node[s];
        return get_node(n, ++i);
        YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(YAML::Node()))
    };
    return get_node(baseNode, 0);
}

YAML::Node YamlConfig::nodeGet(const YAML::Node& baseNode,
                               const std::string& name, bool logWarn) const
{
    YAML::Node node = getNode(baseNode, name);
    if (node.IsNull() && logWarn)
    {
        log_warn_m << "Parameter '" << (_nameNodeFunc + name) << "' is undefined"
                   << ". Config file: " << _filePath;
    }
    return node;
}

YAML::Node YamlConfig::nodeSet(YAML::Node& baseNode, const std::string& name)
{
    std::vector<std::string> parts = utl::split(name, '.');

    typedef std::function<YAML::Node (YAML::Node&, size_t)> NodeFunc;
    NodeFunc get_node = [&](YAML::Node& node, size_t i)
    {
        if (i == parts.size())
            return node;

        YAML_CONFIG_TRY
        std::string s = parts[i];
        YAML::Node n = node[s];
        if (n.IsDefined() && !n.IsMap() && i < (parts.size() - 1))
        {
            node.remove(s);
            n = node[s];
        }
        return get_node(n, ++i);
        YAML_CONFIG_CATCH(YAML_SET_FUNC, YAML_RETURN(YAML::Node()))
    };
    //return get_node(_root, 0);
    return get_node(baseNode, 0);
}

bool YamlConfig::getValue(const std::string& name, Func func, bool logWarn) const
{
    return getValue(_root, name, func, logWarn);
}

bool YamlConfig::getValue(const YAML::Node& baseNode,
                          const std::string& name, Func func, bool logWarn) const
{
    std::lock_guard<std::recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeGet(baseNode, name, logWarn);
    if (!node || node.IsNull())
        return false;

    bool res = false;
    YAML_CONFIG_TRY
    _nameNodeFunc = name + ".";
    res = func(const_cast<YamlConfig*>(this), node, logWarn);
    _nameNodeFunc.clear();
    YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(false))
    return res;
}

bool YamlConfig::setValue(const std::string& name, Func func)
{
    return setValue(_root, name, func);
}

bool YamlConfig::setValue(YAML::Node& baseNode,
                          const std::string& name, Func func)
{
    YAML_CONFIG_CHECK_READONLY

    std::lock_guard<std::recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeSet(baseNode, name);
    bool res = false;
    YAML_CONFIG_TRY
    _nameNodeFunc = name + ".";
    res = func(this, node, false);
    _nameNodeFunc.clear();
    YAML_CONFIG_CATCH(YAML_SET_FUNC, YAML_RETURN(false))
    _changed = true;
    return res;
}

YamlConfigLock::YamlConfigLock(const YamlConfig& yc)
{
    _configLock = &yc._configLock;
    _configLock->lock();
}

YamlConfigLock::~YamlConfigLock()
{
    _configLock->unlock();
}

#undef YAML_CONFIG_CATCH_2

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
