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

namespace yaml {

#define YAML_CONFIG_CATCH_2 \
    } catch (YAML::Exception& e) { \
        alog::Line logLine = log_error_m << "YAML error. Detail: " << e.what(); \
        if (!_filePath.empty()) \
            logLine << ". File: " << _filePath; \
        return false; \
    } catch (exception& e) { \
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

bool Config::readFile(const string& filePath)
{
    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
    _filePath = filePath;
    ifstream file(_filePath, ifstream::in);
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

bool Config::readString(const string& str)
{
    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

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

bool Config::rereadFile()
{
    return readFile(_filePath);
}

bool Config::changed() const
{
    return _changed;
}

bool Config::readOnly() const
{
    return _readOnly;
}

void Config::setReadOnly(bool val)
{
    _readOnly = val;
}

bool Config::saveDisabled() const
{
    return _saveDisabled;
}

void Config::setSaveDisabled(bool val)
{
    _saveDisabled = val;
}

bool Config::save(const string& filePath, YAML::EmitterStyle::value nodeStyle)
{
    if (_saveDisabled)
    {
        log_warn_m << "Save data is disabled. File: " << _filePath;
        return false;
    }

    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
    if (!filePath.empty())
        _filePath = filePath;

    if (_filePath.empty())
    {
        log_error_m << "Cannot save data. Undefined file path";
        return false;
    }

    typedef chrono::high_resolution_clock clock;
    uint64_t timeTick = clock::now().time_since_epoch().count();
    string fileTmp = _filePath + ".tmp" + utl::toString(timeTick);
    remove(fileTmp.c_str());

    try
    {
        ofstream file {fileTmp, ios_base::out};
        if (!file.is_open())
        {
            log_error_m << "Cannot open temporary file " << fileTmp << " for write";
            remove(fileTmp.c_str());
            return false;
        }

        _root.SetStyle(nodeStyle);
        file << _root;
        file.flush();
        file.close();
    }
    catch (...)
    {
        remove(fileTmp.c_str());
        throw;
    }

    if (0 != remove(_filePath.c_str()))
        if (errno != ENOENT)
        {
            log_error_m << "Failed remove old data file " << _filePath;
            remove(fileTmp.c_str());
            return false;
        }

    if (0 != rename(fileTmp.c_str(), _filePath.c_str()))
    {
        log_error_m << "Failed rename temporary file " << fileTmp
                    << " to " << _filePath;
        return false;
    }
    YAML_CONFIG_CATCH_2
    _changed = false;
    return true;
}

string Config::filePath() const
{
    return _filePath;
}

bool Config::remove(const string& name, bool logWarn)
{
    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
     size_t pos = name.find_last_of('.');
     if (pos == string::npos)
     {
         _root.remove(name);
         return true;
     }
     string namePre = name.substr(0, pos);
     string nameKey = name.substr(pos + 1);

     YAML::Node node = getNode(namePre);
     if (!node || node.IsNull())
         return false;

    node.remove(nameKey);
    YAML_CONFIG_CATCH_2
    return true;
}

YAML::Node Config::getNode(const string& name) const
{
    return getNode(_root, name);
}

YAML::Node Config::getNode(const YAML::Node& baseNode, const string& name) const
{
    if (name == ".")
        return baseNode;

    vector<string> parts = utl::split(name, '.');
    if (parts.empty())
        return YAML::Node();

    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    typedef function<YAML::Node (const YAML::Node&, size_t)> NodeFunc;
    NodeFunc get_node = [&](const YAML::Node& node, size_t i)
    {
        if (!node.IsDefined() || node.IsNull())
            return YAML::Node();

        if (i == parts.size())
            return node;

        if (!node.IsMap())
            return YAML::Node();

        YAML_CONFIG_TRY
        string s = parts[i];
        YAML::Node n = node[s];
        return get_node(n, ++i);
        YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(YAML::Node()))
    };
    return get_node(baseNode, 0);
}

YAML::Node Config::nodeGet(const YAML::Node& baseNode,
                           const string& name, bool logWarn) const
{
    YAML::Node node = getNode(baseNode, name);
    if (node.IsNull() && logWarn)
    {
        log_warn_m << "Parameter '" << (_nameNodeFunc + name) << "' is undefined"
                   << ". Config file: " << _filePath;
    }
    return node;
}

YAML::Node Config::nodeSet(YAML::Node& baseNode, const string& name)
{
    vector<string> parts = utl::split(name, '.');

    typedef function<YAML::Node (YAML::Node&, size_t)> NodeFunc;
    NodeFunc get_node = [&](YAML::Node& node, size_t i)
    {
        if (i == parts.size())
            return node;

        YAML_CONFIG_TRY
        string s = parts[i];
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

bool Config::getValue(const string& name, Func func, bool logWarn) const
{
    return getValue(_root, name, func, logWarn);
}

bool Config::getValue(const YAML::Node& baseNode,
                      const string& name, Func func, bool logWarn) const
{
    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeGet(baseNode, name, logWarn);
    if (!node || node.IsNull())
        return false;

    bool res = false;
    YAML_CONFIG_TRY
    _nameNodeFunc = name + ".";
    res = func(const_cast<Config*>(this), node, logWarn);
    _nameNodeFunc.clear();
    YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(false))
    return res;
}

bool Config::setValue(const string& name, Func func)
{
    return setValue(_root, name, func);
}

bool Config::setValue(YAML::Node& baseNode,
                      const string& name, Func func)
{
    YAML_CONFIG_CHECK_READONLY

    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

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

ConfigLock::ConfigLock(const Config& yc)
{
    _configLock = &yc._configLock;
    _configLock->lock();
}

ConfigLock::~ConfigLock()
{
    _configLock->unlock();
}

} // namespace yaml
