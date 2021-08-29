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
#include "safe_singleton.h"
#include "logger/logger.h"
#include "logger/format.h"

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
#include "qt/quuidex.h"
#include <QPair>
#include <QString>
#include <QList>
#include <QVector>
#endif

namespace yaml {

using namespace std;

/**
  Механизм чтения и записи конфигурационных файлов в YAML-нотации
*/
class Config
{
public:
    typedef function<bool (Config*, YAML::Node&, bool /*logWarn*/)> Func;

    Config() = default;

    // Читает yaml-структуру из файла. filePath - полное имя файла
    bool readFile(const string& filePath);

    // Читает yaml-структуру из строки
    bool readString(const string&);

    // Перечитывает yaml-структуру из файла
    bool rereadFile();

    // Возвращает TRUE если параметры конфигурации были изменены, устанавливается
    // в FALSE после сохранения данных в файл
    bool changed() const;

    // Определяет, что параметры конфигурации не могут изменяться
    bool readOnly() const;
    void setReadOnly(bool);

    // Запрещает запись данных в файл конфигурации
    bool saveDisabled() const;
    void setSaveDisabled(bool);

    // Сохраняет данные в файл
    bool save(const string& filePath = string(),
              YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Block);

    // Возвращает полное имя файла конфигурации
    string filePath() const;

    // Удаляет ноду по имени
    bool remove(const string& name, bool logWarn = true);

    // Возвращает ноду с именем name
    YAML::Node getNode(const string& name) const;

    // Возвращает ноду с именем name относительно базовой ноды baseNode
    YAML::Node getNode(const YAML::Node& baseNode, const string& name) const;

    // Используется  для  получения  простого  значения  (скаляр)  из  ноды
    // с именем name. Имя может быть составным.  Составное имя записывается
    // следующим образом: 'param1.param2.param3'.
    // Возвращает TRUE если нода с именем name существует и в параметр value
    // успешно записано  значение.  Параметр  logWarn  определяет  нужно  ли
    // выводить сообщения в лог при неудачном считывании данных
    template<typename T>
    bool getValue(const string& name, T& value,
                  bool logWarn = true) const;

    // Возвращает значение относительно базовой ноды baseNode
    template<typename T>
    bool getValue(const YAML::Node& baseNode,
                  const string& name, T& value,
                  bool logWarn = true) const;

    // Используется для получения  списка  значений  из  ноды  с  именем name.
    // Возвращает TRUE если нода с именем  name  существует и параметр  value
    // успешно  заполнен  значениями.  Параметр  logWarn  определяет нужно ли
    // выводить сообщения в лог при неудачном считывании данных
    template<typename T>
    bool getValue(const string& name, vector<T>& value,
                  bool logWarn = true) const;

    // Используется для получения списка значений относительно базовой
    // ноды baseNode
    template<typename T>
    bool getValue(const YAML::Node& baseNode,
                  const string& name, vector<T>& value,
                  bool logWarn = true) const;

#if defined(QT_CORE_LIB)
    // Перегруженная функция, используется для считывания списка значений
    // в QVector
    template<typename T>
    bool getValue(const string& name, QVector<T>& value,
                  bool logWarn = true) const;

    // Используется для получения списка значений относительно базовой
    // ноды baseNode
    template<typename T>
    bool getValue(const YAML::Node& baseNode,
                  const string& name, QVector<T>& value,
                  bool logWarn = true) const;

    // Перегруженная функция, используется для считывания списка значений
    // в QList
    template<typename T>
    bool getValue(const string& name, QList<T>& value,
                  bool logWarn = true) const;

    // Используется для получения списка значений относительно базовой
    // ноды baseNode
    template<typename T>
    bool getValue(const YAML::Node& baseNode,
                  const string& name, QList<T>& value,
                  bool logWarn = true) const;

    template<typename T1, typename T2>
    bool getValue(const string& name, QPair<T1, T2>& value,
                  bool logWarn = true) const;

    template<typename T1, typename T2>
    bool getValue(const YAML::Node& baseNode,
                  const string& name, QPair<T1, T2>& value,
                  bool logWarn = true) const;
#endif

    // Используется  для  получения  значений  для  нод  сложной  конфигурации.
    // В качестве параметра func используется функция или функтор со следующей
    // сигнатурой function(Config* conf, YAML::Node& node, bool logWarn),
    // где:
    //   conf - указатель на текущий конфигуратор; node - читаемая нода;
    //   logWarn - определяет нужно ли выводить сообщения в лог при неудачном
    //   считывании данных
    bool getValue(const string& name, Func func, bool logWarn = true) const;

    // Используется для получения значений для нод сложной конфигурации относи-
    // тельно базовой ноды baseNode
    bool getValue(const YAML::Node& baseNode,
                  const string& name, Func func, bool logWarn = true) const;

    // Используется для записи простого значения (скаляр) в ноду с именем name.
    // Имя может быть составным. Составное имя записывается следующим образом:
    // 'param1.param2.param3'.
    // Возвращает TRUE если значение было удачно записано в ноду
    template<typename T>
    bool setValue(const string& name, const T& value);

    // Устанавливает значение относительно базовой ноды baseNode
    template<typename T>
    bool setValue(YAML::Node& baseNode,
                  const string& name, const T& value);

    // Используется для записи списка значений в ноду с именем name.
    // Возвращает TRUE если список был удачно записан в ноду
    template<typename T>
    bool setValue(const string& name, const vector<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

    // Используется для записи списка значений относительно базовой ноды baseNode
    template<typename T>
    bool setValue(YAML::Node& baseNode,
                  const string& name, const vector<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

#if defined(QT_CORE_LIB)
    // Перегруженная функция, используется для записи списка значений из QVector
    template<typename T>
    bool setValue(const string& name, const QVector<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

    // Используется для записи списка значений относительно базовой ноды baseNode
    template<typename T>
    bool setValue(YAML::Node& baseNode,
                  const string& name, const QVector<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

    // Перегруженная функция, используется для записи списка значений из QList
    template<typename T>
    bool setValue(const string& name, const QList<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

    // Используется для записи списка значений относительно базовой ноды baseNode
    template<typename T>
    bool setValue(YAML::Node& baseNode,
                  const string& name, const QList<T>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

    template<typename T1, typename T2>
    bool setValue(const string& name, QPair<T1, T2>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);

    template<typename T1, typename T2>
    bool setValue(YAML::Node& baseNode,
                  const string& name, QPair<T1, T2>& value,
                  YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Flow);
#endif

    // Используется для записи значений для нод сложной конфигурации,
    // см. описание getValue()
    bool setValue(const string& name, Func func);

    // Используется для записи значений для нод сложной конфигурации относи-
    // тельно базовой ноды baseNode
    bool setValue(YAML::Node& baseNode, const string& name, Func func);

private:
    DISABLE_DEFAULT_COPY(Config)

    template<typename T>
    static char* typeName();

    // Используется для сохранения и чтения строк с типом QString.
    // См. ниже специализацию этой структуры для QString
    template<typename T>
    struct ProxyStdString
    {
        typedef T Type;
        static  T setter(T t) {return t;}
        static  T getter(T t) {return t;}
    };

    // Возвращает ноду по имени 'name'. Если ноды с заданным именем нет
    // в списке, то будет возвращена пустая нода
    YAML::Node nodeGet(const YAML::Node& baseNode, const string& name,
                       bool logWarn) const;

    // Используется в функциях setValue(). Строит иерархию нод согласно
    // заданному параметру 'name'
    YAML::Node nodeSet(YAML::Node& baseNode, const string& name);

    template<typename VectorT>
    bool getValueVect(const YAML::Node& baseNode,
                      const string& name, VectorT& value,
                      bool logWarn = true) const;

    template<typename VectorT>
    bool setValueVect(YAML::Node& baseNode,
                      const string& name, const VectorT& value,
                      YAML::EmitterStyle::value nodeStyle);

private:
    atomic_bool _changed = {false};
    atomic_bool _readOnly = {false};
    atomic_bool _saveDisabled = {false};
    string _filePath;
    YAML::Node _root;
    mutable recursive_mutex _configLock;

    // Вспомогательная переменная, используется для вывода в лог информации
    // по не найденной ноде
    mutable string _nameNodeFunc;

    friend class ConfigLock;
    template<typename T, int> friend T& ::safe_singleton();
};

//---------------------------- Implementation  -------------------------------

template<typename T>
inline char* Config::typeName()
{
#if defined(_MSC_VER)
    return typeid(T).name();
#else
    return abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
}

#define YAML_GET_FUNC 0
#define YAML_SET_FUNC 1
#define YAML_RETURN(VAL) (VAL)

#define YAML_CONFIG_LOG_ERROR(ERROR, GETSET) \
    alog::Line logLine = \
    alog::logger().error(alog_line_location, "YamlConfig") \
        << "Yaml error"; \
    if (GETSET == 0) \
        logLine << ". Failed to get parameter: " << (_nameNodeFunc + name); \
    else \
        logLine << ". Failed to set parameter: " << (_nameNodeFunc + name); \
    logLine << log_format(". Detail: %?. File: %?", ERROR, _filePath);

#define YAML_CONFIG_TRY \
    try {

#define YAML_CONFIG_CATCH(GETSET, RETURN) \
    } catch (YAML::Exception& e) { \
        YAML_CONFIG_LOG_ERROR(e.what(), GETSET) \
        _nameNodeFunc.clear(); \
        return RETURN; \
    } catch (exception& e) { \
        YAML_CONFIG_LOG_ERROR(e.what(), GETSET) \
        _nameNodeFunc.clear(); \
        return RETURN; \
    } catch (...) { \
        YAML_CONFIG_LOG_ERROR("Unknown error", GETSET) \
        _nameNodeFunc.clear(); \
        return RETURN; \
    }

#define YAML_CONFIG_CHECK_READONLY \
        if (_readOnly) {\
            alog::logger().warn(alog_line_location, "YamlConfig") \
                << "Failed to set parameter: " << name \
                << ". Config data is read only"; \
            return false;  \
        }

#if defined(QT_CORE_LIB)
template<>
struct Config::ProxyStdString<QString>
{
    typedef string Type;
    static Type setter(const QString& s) {return Type(s.toUtf8().constData());}
    static QString getter(const Type& s) {return QString::fromUtf8(s.c_str());}
};
template<>
struct Config::ProxyStdString<QUuid>
{
    typedef string Type;
    static Type setter(const QUuid& u) {
        QByteArray ba = u.toByteArray(); ba.remove(0, 1); ba.chop(1);
        return Type(ba.constData());
    }
    static QUuid getter(const Type& s) {
        return QUuid(QByteArray::fromRawData(s.c_str(), int(s.size())));
    }
};
template<>
struct Config::ProxyStdString<QUuidEx>
{
    typedef string Type;
    static Type setter(const QUuidEx& u) {return ProxyStdString<QUuid>::setter(u);}
    static QUuidEx getter(const Type& s) {return ProxyStdString<QUuid>::getter(s);}
};
#endif

template<typename T>
bool Config::getValue(const string& name, T& value, bool logWarn) const
{
    return getValue(_root, name, value, logWarn);
}

template<typename T>
bool Config::getValue(const YAML::Node& baseNode,
                          const string& name, T& value, bool logWarn) const
{
    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeGet(baseNode, name, logWarn);
    if (!node || node.IsNull())
        return false;

    if (!node.IsScalar())
    {
        if (logWarn)
            alog::logger().warn(alog_line_location, "Config")
                << "Parameter " << (_nameNodeFunc + name) << " is not scalar type";
        return false;
    }

    YAML_CONFIG_TRY
    // было так: value = node.as<T>();
    value = ProxyStdString<T>::getter(node.as<typename ProxyStdString<T>::Type>());
    YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(false))
    return true;
}

template<typename VectorT>
bool Config::getValueVect(const YAML::Node& baseNode,
                              const string& name, VectorT& value,
                              bool logWarn) const
{
    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeGet(baseNode, name, logWarn);
    if (!node || node.IsNull())
        return false;

    if (!node.IsSequence())
    {
        if (logWarn)
            alog::logger().warn(alog_line_location, "Config")
                << "Parameter " << name << " is not sequence type";
        return false;
    }

    VectorT v;
    for (const YAML::Node& n: node)
    {
        if (!n.IsScalar())
        {
            if (logWarn)
                alog::logger().warn(alog_line_location, "Config")
                    << "Parameter " << name
                    << ". The elements of sequence are not a scalar type";
            return false;
        }
        YAML_CONFIG_TRY
        typedef typename VectorT::value_type ValueType;
        // было так: v.push_back(n.as<ValueType>());
        v.push_back(ProxyStdString<ValueType>::getter(
                        n.as<typename ProxyStdString<ValueType>::Type>()));
        YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(false))
    }
    value.swap(v);
    return true;
}

template<typename T>
bool Config::getValue(const string& name, vector<T>& value,
                          bool logWarn) const
{
    return getValueVect(_root, name, value, logWarn);
}

template<typename T>
bool Config::getValue(const YAML::Node& baseNode,
                          const string& name, vector<T>& value,
                          bool logWarn) const
{
    return getValueVect(baseNode, name, value, logWarn);
}

#if defined(QT_CORE_LIB)
template<typename T>
bool Config::getValue(const string& name, QVector<T>& value,
                          bool logWarn) const
{
    return getValueVect(_root, name, value, logWarn);
}

template<typename T>
bool Config::getValue(const YAML::Node& baseNode,
                          const string& name, QVector<T>& value,
                          bool logWarn) const
{
    return getValueVect(baseNode, name, value, logWarn);
}

template<typename T>
bool Config::getValue(const string& name, QList<T>& value,
                          bool logWarn) const
{
    return getValueVect(_root, name, value, logWarn);
}

template<typename T>
bool Config::getValue(const YAML::Node& baseNode,
                          const string& name, QList<T>& value,
                          bool logWarn) const
{
    return getValueVect(baseNode, name, value, logWarn);
}

template<typename T1, typename T2>
bool Config::getValue(const string& name, QPair<T1, T2>& value,
                          bool logWarn) const
{
    return getValue(_root, name, value, logWarn);
}

template<typename T1, typename T2>
bool Config::getValue(const YAML::Node& baseNode,
                          const string& name, QPair<T1, T2>& value,
                          bool logWarn) const
{
    Config::Func func = [&value](Config* conf, YAML::Node& node, bool logWarn)
    {
        bool r1 = conf->getValue(node, "first",  value.first,  logWarn);
        bool r2 = conf->getValue(node, "second", value.second, logWarn);
        return r1 && r2;
    };
    return getValue(baseNode, name, func, logWarn);
}
#endif

template<typename T>
bool Config::setValue(const string& name, const T& value)
{
    return setValue(_root, name, value);
}

template<typename T>
bool Config::setValue(YAML::Node& baseNode,
                          const string& name, const T& value)
{
    YAML_CONFIG_CHECK_READONLY

    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeSet(baseNode, name);
    YAML_CONFIG_TRY
    node = YAML::Node(ProxyStdString<T>::setter(value));
    YAML_CONFIG_CATCH(YAML_SET_FUNC, YAML_RETURN(false))
    _changed = true;
    return true;
}

template<typename VectorT>
bool Config::setValueVect(YAML::Node& baseNode,
                              const string& name, const VectorT& value,
                              YAML::EmitterStyle::value nodeStyle)
{
    YAML_CONFIG_CHECK_READONLY

    lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeSet(baseNode, name);
    YAML_CONFIG_TRY
    node = YAML::Node();
    node.SetStyle(nodeStyle);
    typedef typename VectorT::value_type ValueType;
    for (const ValueType& v : value)
        node.push_back(ProxyStdString<ValueType>::setter(v));
    YAML_CONFIG_CATCH(YAML_SET_FUNC, YAML_RETURN(false))
    _changed = true;
    return true;
}

template<typename T>
bool Config::setValue(const string& name, const vector<T>& value,
                          YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(_root, name, value, nodeStyle);
}

template<typename T>
bool Config::setValue(YAML::Node& baseNode,
                          const string& name, const vector<T>& value,
                          YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(baseNode, name, value, nodeStyle);
}

#if defined(QT_CORE_LIB)
template<typename T>
bool Config::setValue(const string& name, const QVector<T>& value,
                          YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(_root, name, value, nodeStyle);
}

template<typename T>
bool Config::setValue(YAML::Node& baseNode,
                          const string& name, const QVector<T>& value,
                          YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(baseNode, name, value, nodeStyle);
}

template<typename T>
bool Config::setValue(const string& name, const QList<T>& value,
                          YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(_root, name, value, nodeStyle);
}

template<typename T>
bool Config::setValue(YAML::Node& baseNode,
                          const string& name, const QList<T>& value,
                          YAML::EmitterStyle::value nodeStyle)
{
    return setValueVect(baseNode, name, value, nodeStyle);
}

template<typename T1, typename T2>
bool Config::setValue(const string& name, QPair<T1, T2>& value,
                          YAML::EmitterStyle::value nodeStyle)
{
    return setValue(_root, name, value, nodeStyle);
}

template<typename T1, typename T2>
bool Config::setValue(YAML::Node& baseNode,
                          const string& name, QPair<T1, T2>& value,
                          YAML::EmitterStyle::value nodeStyle)
{
    Config::Func func = [&value, nodeStyle]
                            (Config* conf, YAML::Node& node, bool /*logWarn*/)
    {
        node.SetStyle(nodeStyle);
        bool r1 = conf->setValue(node, "first",  value.first);
        bool r2 = conf->setValue(node, "second", value.second);
        return r1 && r2;
    };
    return setValue(baseNode, name, func);
}
#endif

} // namespace yaml

typedef yaml::Config YamlConfig;
