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

#pragma once

#include "defmac.h"
#include "logger/logger.h"
#include "safe_singleton.h"
#include "utils.h"

#include <yaml-cpp/yaml.h>
#include <atomic>
#include <mutex>
#include <exception>
#include <string>
#include <vector>
#include <functional>
#if !defined(_MSC_VER)
#include <cxxabi.h>
#endif

#if defined(QT_CORE_LIB)
#include <QString>
#include <QVector>
#endif

/**
  Механизм чтения и записи конфигурационных файлов в YAML-нотации
*/
class YamlConfig
{
public:
    typedef std::function<bool (YamlConfig*, YAML::Node&, bool /*logWarn*/)> Func;

    YamlConfig() = default;

    // Читает yaml-структуру из файла. filePath - полное имя файла
    bool readFile(const std::string& filePath);

    // Читает yaml-структуру из строки
    bool readString(const std::string&);

    // Перечитывает yaml-структуру из файла
    bool rereadFile();

    // Определяет, что параметры конфигурации не могут изменяться
    bool readOnly() const;
    void setReadOnly(bool);

    // Запрещает запись данных в файл конфигурации
    bool saveDisabled() const;
    void setSaveDisabled(bool);

    // Сохраняет данные в файл
    bool save(const std::string& filePath = std::string(),
              YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Block);

    // Возвращает полное имя файла конфигурации
    std::string filePath() const;

    // Удаляет ноду по имени
    bool remove(const std::string& name, bool logWarn = true);

    // Возвращает ноду с именем name
    YAML::Node getNode(const std::string& name) const;

    // Возвращает ноду с именем name относительно базовой ноды baseNode
    YAML::Node getNode(const YAML::Node& baseNode, const std::string& name) const;

    // Используется для получения простого значения (скаляр) из ноды
    // с именем name. Имя может быть составным. Составное имя записывается
    // следующим образом: 'param1.param2.param3'.
    // Возвращает TRUE если нода с именем 'name' существует и в параметр value
    // было успешно записано значение.  Если параметр  logWarn  равен TRUE, то
    // в случае неудачного считывания данных лог выводится сообщение о причинах.
    template<typename T>
    bool getValue(const std::string& name, T& value, bool logWarn = true) const;

    // Возвращает значение относительно базовой ноды baseNode
    template<typename T>
    bool getValue(const YAML::Node& baseNode, const std::string& name,
                  T& value, bool logWarn = true) const;

    // Используется для получения списка значений из ноды с именем name.
    // Возвращает TRUE если нода с именем 'name' существует и в параметр value
    // был успешно заполнен значениями. Если параметр logWarn равен TRUE,
    // то в случае неудачного считывания данных лог выводится сообщение
    // о причинах.
    template<typename T>
    bool getValue(const std::string& name,
                  std::vector<T>& value, bool logWarn = true) const;

    // Используется для получения списка значений относительно базовой
    // ноды baseNode
    template<typename T>
    bool getValue(const YAML::Node& baseNode, const std::string& name,
                  std::vector<T>& value, bool logWarn = true) const;

#if defined(QT_CORE_LIB)
    // Перегруженная функция, используется для считывания списка значений
    // в QVector.
    template<typename T>
    bool getValue(const std::string& name,
                  QVector<T>& value, bool logWarn = true) const;

    // Используется для получения списка значений относительно базовой
    // ноды baseNode
    template<typename T>
    bool getValue(const YAML::Node& baseNode, const std::string& name,
                  QVector<T>& value, bool logWarn = true) const;
#endif

    // Используется для получения значений для нод сложной конфигурации.
    // В качестве параметра func используется функция или функтор со следующей
    // сигнатурой function(YamlConfig* conf, YAML::Node& node, bool logWarn),
    // где: conf - указатель на текущий конфигуратор; node - читаемая нода;
    // logWarn - определяет нужно ли выводить сообщения в лог в случае неудач-
    // ного считывания данных.
    bool getValue(const std::string& name, Func func, bool logWarn = true) const;

    // Используется для записи простого значения (скаляр) в ноду с именем name.
    // Имя может быть составным. Составное имя записывается следующим образом:
    // 'param1.param2.param3'.
    // Возвращает TRUE если значение было удачно записано в ноду.
    template<typename T>
    bool setValue(const std::string& name, const T& value);

    // Устанавливает значение относительно базовой ноды baseNode
    template<typename T>
    bool setValue(YAML::Node& baseNode,
                  const std::string& name, const T& value);

    // Используется для записи списка значений в ноду с именем name.
    // Возвращает TRUE если список был удачно записан в ноду.
    template<typename T>
    bool setValue(const std::string& name, const std::vector<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

    // Используется для записи списка значений относительно базовой ноды baseNode
    template<typename T>
    bool setValue(YAML::Node& baseNode,
                  const std::string& name, const std::vector<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

#if defined(QT_CORE_LIB)
    // Перегруженная функция, используется для записи списка значений
    // из QVector.
    template<typename T>
    bool setValue(const std::string& name, const QVector<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

    // Используется для записи списка значений относительно базовой ноды baseNode
    template<typename T>
    bool setValue(YAML::Node& baseNode,
                  const std::string& name, const QVector<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);
#endif

    // Используется для записи значений для нод сложной конфигурации,
    // см. описание getValue().
    bool setValue(const std::string& name, Func func);

private:
    DISABLE_DEFAULT_COPY(YamlConfig)

    template<typename T>
    static char* typeName();

    // Используется для того чтобы можно было сохранять/загружать в конфиг
    // строки QString. См. ниже специализацию этой структуры для QString.
    template<typename T>
    struct ProxyStdString
    {
        typedef T Type;
        static  T setter(T t) {return t;}
        static  T getter(T t) {return t;}
    };

    // Возвращает ноду по имени 'name'. Если ноды с заданным именем нет
    // в списке, то будет возвращена пустая нода.
    YAML::Node nodeGet(const YAML::Node& baseNode,
                       const std::string& name, bool logWarn) const;

    // Используется в функциях setValue(). Строит иерархию нод согласно
    // заданному параметру 'name'.
    YAML::Node nodeSet(YAML::Node& baseNode, const std::string& name);

    template<typename VectorT>
    bool getValueVect(const YAML::Node& baseNode, const std::string& name,
                      VectorT& value, bool logWarn = true) const;

    template<typename VectorT>
    bool setValueVect(YAML::Node& baseNode, const std::string& name,
                      const VectorT& value, YAML::EmitterStyle::value nodeStyle);

private:
    std::atomic_bool _readOnly = {false};
    std::atomic_bool _saveDisabled = {false};
    std::string _filePath;
    YAML::Node _root;
    mutable std::recursive_mutex _configLock;

    // Вспомогательная переменная, используется для вывода в лог информации
    // по не найденной ноде
    mutable std::string _nameNodeFunc;

    friend class YamlConfigLock;
    template<typename T, int> friend T& ::safe_singleton();
};

class YamlConfigLock
{
public:
    YamlConfigLock(const YamlConfig&);
    ~YamlConfigLock();

private:
    DISABLE_DEFAULT_FUNC(YamlConfigLock)
    std::recursive_mutex* _configLock;
};

//---------------------------- Implementation  -------------------------------

template<typename T>
inline char* YamlConfig::typeName()
{
#if defined(_MSC_VER)
    return typeid(T).name();
#else
    return abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
}

#define YAMLGET_FUNC 0
#define YAMLSET_FUNC 1
#define YAMLRETURN(VAL) (VAL)

#define YAMLCONFIG_LOG_ERROR(ERROR, GETSET) \
    alog::Line logLine = \
    alog::logger().error(alog_line_location, "YamlConfig") \
        << "Yaml error"; \
    if (GETSET == 0) \
        logLine << ". Failed to get parameter: " << (_nameNodeFunc + name); \
    else \
        logLine << ". Failed to set parameter: " << (_nameNodeFunc + name); \
    logLine << ". Detail: " << ERROR \
            << ". File: " << _filePath;

#define YAMLCONFIG_TRY \
    try {

#define YAMLCONFIG_CATCH(GETSET, RETURN) \
    } catch (YAML::Exception& e) { \
        YAMLCONFIG_LOG_ERROR(e.what(), GETSET) \
        _nameNodeFunc.clear(); \
        return RETURN; \
    } catch (std::exception& e) { \
        YAMLCONFIG_LOG_ERROR(e.what(), GETSET) \
        _nameNodeFunc.clear(); \
        return RETURN; \
    } catch (...) { \
        YAMLCONFIG_LOG_ERROR("Unknown error", GETSET) \
        _nameNodeFunc.clear(); \
        return RETURN; \
    }

#define YAMLCONFIG_CHECK_READONLY \
        if (_readOnly) {\
            alog::logger().warn(alog_line_location, "YamlConfig") \
                << "Failed to set parameter: " << name \
                << ". Config data is read only"; \
            return false;  \
        }

#if defined(QT_CORE_LIB)
template<>
struct YamlConfig::ProxyStdString<QString>
{
    typedef std::string Type;
    static Type setter(const QString& s) {return Type(s.toUtf8().constData());}
    static QString getter(const Type& s) {return QString::fromUtf8(s.c_str());}
};
#endif

template<typename T>
bool YamlConfig::getValue(const std::string& name, T& value, bool logWarn) const
{
    return getValue(_root, name, value, logWarn);
}

template<typename T>
bool YamlConfig::getValue(const YAML::Node& baseNode,
                          const std::string& name, T& value, bool logWarn) const
{
    std::lock_guard<std::recursive_mutex> locker(_configLock); (void) locker;

    YAML::Node node = nodeGet(baseNode, name, logWarn);
    if (!node || node.IsNull())
        return false;

    if (!node.IsScalar())
    {
        if (logWarn)
            alog::logger().warn(alog_line_location, "YamlConfig")
                << "Parameter " << (_nameNodeFunc + name) << " is not scalar type";
        return false;
    }

    YAMLCONFIG_TRY
    // было так: value = node.as<T>();
    value = ProxyStdString<T>::getter(node.as<typename ProxyStdString<T>::Type>());
    YAMLCONFIG_CATCH(YAMLGET_FUNC, YAMLRETURN(false))
    return true;
}

template<typename VectorT>
bool YamlConfig::getValueVect(const YAML::Node& baseNode, const std::string& name,
                              VectorT& value, bool logWarn) const
{
    YAML::Node node = nodeGet(baseNode, name, logWarn);
    if (!node || node.IsNull())
        return false;

    if (!node.IsSequence())
    {
        if (logWarn)
            alog::logger().warn(alog_line_location, "YamlConfig")
                << "Parameter " << name << " is not sequence type";
        return false;
    }

    VectorT v;
    for (const YAML::Node& n: node)
    {
        if (!n.IsScalar())
        {
            if (logWarn)
                alog::logger().warn(alog_line_location, "YamlConfig")
                    << "Parameter " << name
                    << ". The elements of sequence are not a scalar type";
            return false;
        }
        YAMLCONFIG_TRY
        typedef typename VectorT::value_type ValueType;
        // было так: v.push_back(n.as<ValueType>());
        v.push_back(ProxyStdString<ValueType>::getter(
                        n.as<typename ProxyStdString<ValueType>::Type>()));
        YAMLCONFIG_CATCH(YAMLGET_FUNC, YAMLRETURN(false))
    }
    value.swap(v);
    return true;
}

template<typename T>
bool YamlConfig::getValue(const std::string& name,
                          std::vector<T>& value, bool logWarn) const
{
    std::lock_guard<std::recursive_mutex> locker(_configLock); (void) locker;
    return getValueVect(_root, name, value, logWarn);
}

template<typename T>
bool YamlConfig::getValue(const YAML::Node& baseNode, const std::string& name,
                          std::vector<T>& value, bool logWarn) const
{
    std::lock_guard<std::recursive_mutex> locker(_configLock); (void) locker;
    return getValueVect(baseNode, name, value, logWarn);
}

#if defined(QT_CORE_LIB)
template<typename T>
bool YamlConfig::getValue(const std::string& name,
                          QVector<T>& value, bool logWarn) const
{
    std::lock_guard<std::recursive_mutex> locker(_configLock); (void) locker;
    return getValueVect(_root, name, value, logWarn);
}

template<typename T>
bool YamlConfig::getValue(const YAML::Node& baseNode, const std::string& name,
                          QVector<T>& value, bool logWarn) const
{
    std::lock_guard<std::recursive_mutex> locker(_configLock); (void) locker;
    return getValueVect(baseNode, name, value, logWarn);
}
#endif

template<typename T>
bool YamlConfig::setValue(const std::string& name, const T& value)
{
    return setValue(_root, name, value);
}

template<typename T>
bool YamlConfig::setValue(YAML::Node& baseNode,
                          const std::string& name, const T& value)
{
    YAMLCONFIG_CHECK_READONLY

    std::lock_guard<std::recursive_mutex> locker(_configLock); (void) locker;

    YAML::Node node = nodeSet(baseNode, name);
    YAMLCONFIG_TRY
    node = YAML::Node(ProxyStdString<T>::setter(value));
    YAMLCONFIG_CATCH(YAMLSET_FUNC, YAMLRETURN(false))
    return true;
}

template<typename VectorT>
bool YamlConfig::setValueVect(YAML::Node& baseNode, const std::string& name,
                              const VectorT& value, YAML::EmitterStyle::value nodeStyle)
{
    YAMLCONFIG_CHECK_READONLY

    std::lock_guard<std::recursive_mutex> locker(_configLock); (void) locker;

    YAML::Node node = nodeSet(baseNode, name);
    YAMLCONFIG_TRY
    node = YAML::Node();
    node.SetStyle(nodeStyle);
    typedef typename VectorT::value_type ValueType;
    for (const ValueType& v : value)
        node.push_back(ProxyStdString<ValueType>::setter(v));
    YAMLCONFIG_CATCH(YAMLSET_FUNC, YAMLRETURN(false))
    return true;
}

template<typename T>
bool YamlConfig::setValue(const std::string& name,
                          const std::vector<T>& value, YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(_root, name, value, nodeStyle);
}

template<typename T>
bool YamlConfig::setValue(YAML::Node& baseNode, const std::string& name,
                          const std::vector<T>& value, YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(baseNode, name, value, nodeStyle);
}

#if defined(QT_CORE_LIB)
template<typename T>
bool YamlConfig::setValue(const std::string& name,
                          const QVector<T>& value, YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(_root, name, value, nodeStyle);
}

template<typename T>
bool YamlConfig::setValue(YAML::Node& baseNode, const std::string& name,
                          const QVector<T>& value, YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(baseNode, name, value, nodeStyle);
}
#endif

