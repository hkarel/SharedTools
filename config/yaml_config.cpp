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
#include "prog_abort.h"
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

#define YAML_CONFIG_CATCH_FILE \
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

#define YAML_CONFIG_CATCH_STRING \
    } catch (YAML::Exception& e) { \
        alog::Line logLine = log_error_m << "YAML error. Detail: " << e.what(); \
        return false; \
    } catch (exception& e) { \
        alog::Line logLine = log_error_m << "YAML error. Detail: " << e.what(); \
        return false; \
    } catch (...) { \
        alog::Line logLine = log_error_m << "YAML error. Detail: Unknown error"; \
        return false; \
    }

Config::Locker::Locker(const Config* c) : config(c)
{
    config->_configLock.lock();
    ++config->_lockedCount;
}
Config::Locker::~Locker()
{
    config->_configLock.unlock();
    --config->_lockedCount;
}

bool Config::readFile(const string& filePath)
{
    Locker locker {this}; (void) locker;
    //lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
    _filePath = filePath;
    ifstream file(_filePath, ifstream::in);
    if (!file.is_open())
    {
        log_warn_m << "Cannot open config file: " << _filePath;
        _root = YAML::Node();
        return false;
    }
    _root = YAML::Load(file);
    if (!_root.IsDefined())
    {
        log_error_m << "Undefined YAML-structure for config file: " << filePath;
        return false;
    }
    YAML_CONFIG_CATCH_FILE
    _changed = false;
    return true;
}

bool Config::readString(const string& str)
{
    Locker locker {this}; (void) locker;
    //lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
    _filePath.clear();
    _root = YAML::Load(str);
    if (!_root.IsDefined())
    {
        log_error_m << "Undefined YAML-structure in input string";
        return false;
    }
    YAML_CONFIG_CATCH_STRING
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

void Config::resetChanged()
{
    _changed = false;
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

bool Config::saveFile(const string& filePath, YAML::EmitterStyle::value nodeStyle)
{
    if (_saveDisabled)
    {
        log_warn_m << "Save data is disabled. File: " << _filePath;
        return false;
    }

    Locker locker {this}; (void) locker;
    //lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

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

    // Не удалять пространство имен std у функции remove(),  т.к. это приведет
    // к переопределению вызова функции remove() при сборке под  MinGW.  Будет
    // вызываться функция член-класса, вместо системной функции, как следствие
    // механизм сохранения корректно работать не будет
    std::remove(fileTmp.c_str());

    try
    {
        ofstream file {fileTmp, ios_base::out};
        if (!file.is_open())
        {
            log_error_m << log_format(
                "Cannot open temporary file %? for write", fileTmp);
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
            log_error_m << log_format(
                "Failed remove old data file %?. Error code: %?",
                _filePath, errno);
            std::remove(fileTmp.c_str());
            return false;
        }

    if (0 != std::rename(fileTmp.c_str(), _filePath.c_str()))
    {
        log_error_m << log_format(
            "Failed rename temporary file %? to %?. Error code: %?",
            fileTmp, _filePath, errno);
        return false;
    }
    YAML_CONFIG_CATCH_FILE
    _changed = false;
    return true;
}

bool Config::saveString(const string& str, YAML::EmitterStyle::value nodeStyle)
{
    if (_saveDisabled)
    {
        log_warn_m << "Save data is disabled";
        return false;
    }

    Locker locker {this}; (void) locker;
    //lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
    ostringstream stream {str, ios_base::out};
    _root.SetStyle(nodeStyle);
    stream << _root;
    YAML_CONFIG_CATCH_STRING
    return true;
}

string Config::filePath() const
{
    return _filePath;
}

bool Config::remove(const string& name, bool /*logWarn*/)
{
    Locker locker {this}; (void) locker;
    //lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML_CONFIG_TRY
     size_t pos = name.find_last_of('.');
     if (pos == string::npos)
     {
         _root.remove(name);
         return true;
     }
     string namePre = name.substr(0, pos);
     string nameKey = name.substr(pos + 1);

     YAML::Node node = this->node(namePre);
     if (!node || node.IsNull())
         return false;

    node.remove(nameKey);
    YAML_CONFIG_CATCH_FILE
    return true;
}

YAML::Node Config::node(const string& name) const
{
    return node(_root, name);
}

YAML::Node Config::node(const YAML::Node& baseNode, const string& name) const
{
    if (name == ".")
        return baseNode;

    vector<string> parts = utl::split(name, '.');
    if (parts.empty())
        return YAML::Node();

    if (_lockedCount == 0)
    {
        log_error_m << "Yaml config not locked. Use function Config::locker()";
        prog_abort();
    }
    //lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

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

YAML::Node Config::nodeGet(const string& name, bool logWarn) const
{
    return nodeGet(_root, name, logWarn);
}

YAML::Node Config::nodeGet(const YAML::Node& baseNode, const string& name,
                           bool logWarn) const
{
    YAML::Node node = this->node(baseNode, name);
    if (node.IsNull() && logWarn)
    {
        log_warn_m << log_format("Parameter '%?' is undefined. Config file: %?",
                                 _nameNodeFunc + name, _filePath);
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
    return get_node(baseNode, 0);
}

YAML::EmitterStyle::value Config::nodeStyle(const std::string& name) const
{
    return nodeStyle(_root, name);
}

YAML::EmitterStyle::value Config::nodeStyle(const YAML::Node& baseNode,
                                            const std::string& name) const
{
    Locker locker {this}; (void) locker;
    //std::lock_guard<std::recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeGet(baseNode, name, true);
    if (!node || node.IsNull())
        return YAML::EmitterStyle::value::Default;

    YAML::EmitterStyle::value style;
    YAML_CONFIG_TRY
    style = node.Style();
    YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(YAML::EmitterStyle::value::Default))
    return style;
}

void Config::setNodeStyle(const std::string& name, YAML::EmitterStyle::value style)
{
    setNodeStyle(_root, name, style);
}

void Config::setNodeStyle(YAML::Node& baseNode, const std::string& name,
                          YAML::EmitterStyle::value style)
{
    if (_readOnly)
    {
        log_warn_m << "Failed set node style for " << name
                   << ". Config data is read only";
        return;
    }

    Locker locker {this}; (void) locker;
    //std::lock_guard<std::recursive_mutex> locker {_configLock}; (void) locker;

    // Исходим из того, что стиль устанавливается для уже существующей ноды,
    // поэтому вызываем nodeGet()
    YAML::Node node = nodeGet(baseNode, name, true);
    if (!node || node.IsNull())
        return;

    YAML_CONFIG_TRY
    node.SetStyle(style);
    YAML_CONFIG_CATCH(YAML_SET_FUNC, YAML_RETURN((void)0))
}

bool Config::getValueInternal(const YAML::Node& node, const string& name,
                              Func func, bool logWarn) const
{
    bool res = false;
    YAML_CONFIG_TRY
    _nameNodeFunc = name + ".";
    res = func(const_cast<Config*>(this), const_cast<YAML::Node&>(node), logWarn);
    _nameNodeFunc.clear();
    YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(false))
    return res;
}

bool Config::getValueInternal(const YAML::Node& node, const string& name,
                              QPoint& value, bool logWarn) const
{
    Config::Func func = [&value](Config* c, YAML::Node& n, bool logWarn)
    {
        int x, y;
        bool r1 = c->getValue(n, "x", x, logWarn);
        bool r2 = c->getValue(n, "y", y, logWarn);
        if (r1 && r2)
            value = {x, y};

        return r1 && r2;
    };
    return getValueInternal(node, name, func, logWarn);
}

bool Config::getValueInternal(const YAML::Node& node, const string& name,
                              QSize& value, bool logWarn) const
{
    Config::Func func = [&value](Config* c, YAML::Node& n, bool logWarn)
    {
        int width, height;
        bool r1 = c->getValue(n, "width",  width,  logWarn);
        bool r2 = c->getValue(n, "height", height, logWarn);
        if (r1 && r2)
            value = {width, height};

        return r1 && r2;
    };
    return getValueInternal(node, name, func, logWarn);
}

bool Config::setValueInternal(YAML::Node& node, const string& name, Func func)
{
    bool res = false;
    YAML_CONFIG_TRY
    _nameNodeFunc = name + ".";
    res = func(this, node, false);
    _nameNodeFunc.clear();
    YAML_CONFIG_CATCH(YAML_SET_FUNC, YAML_RETURN(false))
    _changed = true;
    return res;
}

bool Config::setValueInternal(YAML::Node& node, const string& name,
                              const QPoint& value)
{
    Config::Func func = [&value] (Config* c, YAML::Node& n, bool /*logWarn*/)
    {
        n.SetStyle(YAML::EmitterStyle::Flow);
        bool r1 = c->setValue(n, "x", value.x());
        bool r2 = c->setValue(n, "y", value.y());
        return r1 && r2;
    };
    return setValueInternal(node, name, func);
}

bool Config::setValueInternal(YAML::Node& node, const string& name,
                              const QSize& value)
{
    Config::Func func = [&value] (Config* c, YAML::Node& n, bool /*logWarn*/)
    {
        n.SetStyle(YAML::EmitterStyle::Flow);
        bool r1 = c->setValue(n, "width",  value.width());
        bool r2 = c->setValue(n, "height", value.height());
        return r1 && r2;
    };
    return setValueInternal(node, name, func);
}

} // namespace yaml
