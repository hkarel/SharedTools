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
#include <fstream>
#include <stdexcept>
#include <errno.h>

#if defined(QT_CORE_LIB)
#include <QByteArray>
#endif

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig")

#define YAMLCONFIG_CATCH_2 \
    } catch (YAML::Exception& e) { \
        log_error_m << "YAML error. Detail: " << e.what() \
                    << ". File: " << _filePath; \
        return false; \
    } catch (std::exception& e) { \
        log_error_m << "YAML error. Detail: " << e.what() \
                    << ". File: " << _filePath; \
        return false; \
    } catch (...) { \
        log_error_m << "YAML error. Detail: Unknown error" \
                    << ". File: " << _filePath; \
        return false; \
    }


bool YamlConfig::read(const std::string& filePath)
{
    std::lock_guard<std::mutex> locker(_configLock); (void) locker;

    YAMLCONFIG_TRY
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
    YAMLCONFIG_CATCH_2
    return true;
}

bool YamlConfig::reRead()
{
    return read(_filePath);
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

bool YamlConfig::save(const std::string& filePath)
{
    if (_saveDisabled)
    {
        log_warn_m << "Save the data is disabled. File: " << _filePath;
        return false;
    }

    std::lock_guard<std::mutex> locker(_configLock); (void) locker;

    YAMLCONFIG_TRY
    if (!filePath.empty())
        _filePath = filePath;

    if (_filePath.empty())
    {
        log_error_m << "Cannot save data. Undefined file path";
        return false;
    }

    std::srand(std::time(0));
    std::string fileTmp = _filePath + ".tmp" + utl::toString(std::rand());
    std::remove(fileTmp.c_str());

    try
    {
        std::ofstream file(fileTmp, std::ios_base::out);
        if (!file.is_open())
        {
            log_error_m << "Cannot open temporary file " << fileTmp << " for write";
            std::remove(fileTmp.c_str());
            return false;
        }

        _root.SetStyle(YAML::EmitterStyle::Block);
        file << _root;
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
    YAMLCONFIG_CATCH_2
    return true;
}

std::string YamlConfig::filePath() const
{
    return _filePath;
}

bool YamlConfig::remove(const std::string& name, bool logWarnings)
{
    std::lock_guard<std::mutex> locker(_configLock); (void) locker;

    YAMLCONFIG_TRY
     size_t pos = name.find_last_of('.');
     if (pos == std::string::npos)
     {
         _root.remove(name);
         return true;
     }
     std::string namePre = name.substr(0, pos);
     std::string nameKey = name.substr(pos + 1);

     YAML::Node node = nodeGet(namePre, logWarnings);
     if (!node)
         return false;

    node.remove(nameKey);
    YAMLCONFIG_CATCH_2
    return true;
}

YAML::Node YamlConfig::getNode(const std::string& name, bool logWarnings) const
{
    return nodeGet(name, logWarnings);
}

YAML::Node YamlConfig::nodeGet(const std::string& name, bool logWarnings) const
{
    std::vector<std::string> parts = utl::split(name, '.');
    if (parts.empty())
        return YAML::Node();

    typedef std::function<YAML::Node (const YAML::Node&, size_t)> NodeFunc;
    NodeFunc getNode = [&](const YAML::Node& node, size_t i)
    {
        if (i == parts.size())
            return node;

        if (!node.IsDefined())
            return YAML::Node();

        if (i < (parts.size() - 1) && !node.IsMap())
            return YAML::Node();

        YAMLCONFIG_TRY
        std::string s = parts[i];
        YAML::Node n = node[s];
        if (!n.IsDefined())
            return YAML::Node();
        return getNode(n, ++i);
        YAMLCONFIG_CATCH(YAMLGET_FUNC, YAMLRETURN(YAML::Node()))
    };
    return getNode(_root, 0);
}

bool YamlConfig::nodeGet(const std::string& name,
                         YAML::Node& node, bool logWarnings) const
{
    node = nodeGet(name, logWarnings);
    if (!node)
        return false;

    if (node.IsNull())
    {
        if (logWarnings)
            log_warn_m << "Parameter '" << name << "' is undefined"
                       << ". Config file: " << _filePath;
        return false;
    }
    return true;
}

YAML::Node YamlConfig::nodeSet(const std::string& name)
{
    std::vector<std::string> parts = utl::split(name, '.');

    typedef std::function<YAML::Node (YAML::Node&, size_t)> NodeFunc;
    NodeFunc getNode = [&](YAML::Node& node, size_t i)
    {
        if (i == parts.size())
            return node;
        YAMLCONFIG_TRY
        std::string s = parts[i];
        YAML::Node n = node[s];
        return getNode(n, ++i);
        YAMLCONFIG_CATCH(YAMLSET_FUNC, YAMLRETURN(YAML::Node()))
    };
    return getNode(_root, 0);
}

bool YamlConfig::getValue(const std::string& name,
                          Func func, bool logWarnings) const
{
    std::lock_guard<std::mutex> locker(_configLock); (void) locker;

    YAML::Node node;
    if (!nodeGet(name, node, logWarnings))
            return false;

    YAMLCONFIG_TRY
    return func(node, logWarnings);
    YAMLCONFIG_CATCH(YAMLGET_FUNC, YAMLRETURN(false))
}

bool YamlConfig::setValue(const std::string& name, Func func)
{
    YAMLCONFIG_CHECK_READONLY

    std::lock_guard<std::mutex> locker(_configLock); (void) locker;

    YAML::Node node = nodeSet(name);
    YAMLCONFIG_TRY
    return func(node, false);
    YAMLCONFIG_CATCH(YAMLSET_FUNC, YAMLRETURN(false))
}

#undef YAMLCONFIG_CATCH_2

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
