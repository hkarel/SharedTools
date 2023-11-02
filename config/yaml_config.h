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
#include <QList>
#include <QPair>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QVector>
#endif

namespace yaml {

using namespace std;

namespace detail {

#if defined(QT_CORE_LIB)
#define YAML_TYPES(TYPE) is_fundamental<TYPE>::value \
                         || std::is_same<TYPE, std::string>::value \
                         || std::is_same<TYPE, QString>::value \
                         || std::is_same<TYPE, QUuid>::value \
                         || std::is_same<TYPE, QUuidEx>::value
#else
#define YAML_TYPES(TYPE) std::is_fundamental<TYPE>::value \
                         || std::is_same<TYPE, std::string>::value
#endif

template<typename T> using is_yaml_type  = std::enable_if< (YAML_TYPES(T)), int>;
template<typename T> using not_yaml_type = std::enable_if<!(YAML_TYPES(T)), int>;

#undef YAML_TYPES
} // namespace detail

/**
  Механизм чтения и записи конфигурационных файлов в YAML-нотации
*/
class Config
{
public:
    typedef function<bool (Config*, YAML::Node&, bool /*logWarn*/)> Func;

    struct Locker
    {
        ~Locker();
    private:
        DISABLE_DEFAULT_FUNC(Locker)
        Locker(const Config*);
        Config const* config;
        friend class Config;
    };

    // Блокирует конфигуратор для безопасного вызова функций node(), nodeGet().
    // Пример использования:
    //   auto locker {сonfig.locker()}; (void) locker;
    //   YAML::Node node = сonfig.node("node_name");
#ifdef __GNUC__
    Locker locker() const __attribute__ ((warn_unused_result)) {return Locker(this);}
#else
    Locker locker() const {return Locker(this);}
#endif

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

    // Сбрасывает флаг изменений в FALSE. Смотри пояснения для функции saveString()
    void resetChanged();

    // Определяет, что параметры конфигурации не могут изменяться
    bool readOnly() const;
    void setReadOnly(bool);

    // Запрещает запись данных в файл конфигурации
    bool saveDisabled() const;
    void setSaveDisabled(bool);

    // Сохраняет данные в файл, при этом флаг изменений (changed) сбрасывается
    // в FALSE
    bool saveFile(const string& filePath = string(),
            YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Block);

    // Сохраняет данные в строку, при этом флаг изменений (changed) не сбрасывается
    // в FALSE. Для сброса флага изменений нужно явно вызвать функцию resetChanged()
    bool saveString(const string&,
            YAML::EmitterStyle::value nodeStyle = YAML::EmitterStyle::Block);

    // Возвращает полное имя файла конфигурации
    string filePath() const;

    // Удаляет ноду по имени
    bool remove(const string& name, bool logWarn = true);

    // Возвращает ноду с именем name. Результат может быть использован
    // в качестве базовой ноды (baseNode).
    // Функция используется совместно с Config::locker()
    YAML::Node node(const string& name) const;

    // Возвращает ноду с именем name относительно базовой ноды baseNode
    // Функция используется совместно с Config::locker()
    YAML::Node node(const YAML::Node& baseNode, const string& name) const;

    // Возвращает ноду с именем name. Если ноды с заданным именем не существует,
    // будет возвращена пустая нода, а в лог выведено соответствующее сообщение.
    // Функция используется совместно с Config::locker()
    YAML::Node nodeGet(const string& name, bool logWarn = true) const;

    // Возвращает ноду с именем name относительно базовой ноды baseNode.
    // Функция используется совместно с Config::locker()
    YAML::Node nodeGet(const YAML::Node& baseNode, const string& name,
                       bool logWarn = true) const;

    // Возвращает стиль записи для ноды с именем name
    YAML::EmitterStyle::value nodeStyle(const std::string& name) const;

    // Возвращает стиль записи для ноды с именем name относительно базовой ноды
    YAML::EmitterStyle::value nodeStyle(const YAML::Node& baseNode,
                                        const std::string& name) const;

    // Устанавливает стиль записи для ноды с именем name
    void setNodeStyle(const std::string& name, YAML::EmitterStyle::value);

    // Устанавливает стиль записи для ноды с именем name относительно базовой ноды
    void setNodeStyle(YAML::Node& baseNode, const std::string& name,
                      YAML::EmitterStyle::value);

    // Используется для получения значения из ноды с именем  name. Имя может
    // быть составным. Составное имя записывается следующим образом:
    // 'param1.param2.param3'.
    // Возвращает TRUE если нода с именем name существует и в параметр value
    // успешно записано  значение.  Параметр  logWarn  определяет  нужно  ли
    // выводить сообщения в лог при неудачном считывании данных
    template<typename T>
    bool getValue(const string& name, T& value, bool logWarn = true) const;

    // Возвращает значение относительно базовой ноды baseNode
    template<typename T>
    bool getValue(const YAML::Node& baseNode, const string& name, T& value,
                  bool logWarn = true) const;

    // Используется для записи значения в ноду с именем name. Имя может быть
    // составным. Составное имя записывается следующим образом:
    // 'param1.param2.param3'.
    // Возвращает TRUE если значение было удачно записано в ноду
    template<typename T>
    bool setValue(const string& name, const T& value);

    // Устанавливает значение относительно базовой ноды baseNode
    template<typename T>
    bool setValue(YAML::Node& baseNode, const string& name, const T& value);

private:
    DISABLE_DEFAULT_COPY(Config)

    // Использовать abi_type_name()
    // template<typename T>
    // static char* typeName();

    // Используется для преобразования типов к строковому представлению
    template<typename T>
    struct ProxyType
    {
        typedef T Type;
        static  T setter(T t) {return t;}
        static  T getter(T t) {return t;}
    };

    // Используется в функциях setValue(). Строит иерархию нод согласно
    // заданному параметру name
    YAML::Node nodeSet(YAML::Node& baseNode, const string& name);

private:
    template<typename T>
    bool getValueBase(const YAML::Node& node, const string& name, T& value,
                      bool logWarn) const;

    // Используется  для  получения  значений  для  нод  сложной  конфигурации.
    // В качестве параметра func используется функция или функтор со следующей
    // сигнатурой function(Config* conf, YAML::Node& node, bool logWarn),
    // где:
    //   conf - указатель на текущий конфигуратор; node - читаемая нода;
    //   logWarn - определяет нужно ли выводить сообщения в лог при неудачном
    //   считывании данных
    bool getValueInternal(const YAML::Node& node, const string& name,
                          Func func, bool logWarn) const;

    template<typename VectorT>
    bool getValueVector(const YAML::Node& node, const string& name,
                        VectorT& value, bool logWarn) const;

    template<typename T>
    bool getValueInternal(const YAML::Node& node, const string& name,
                          vector<T>& value, bool logWarn) const;

#if defined(QT_CORE_LIB)

#if QT_VERSION < 0x060000
    template<typename T>
    bool getValueInternal(const YAML::Node& node, const string& name,
                          QVector<T>& value, bool logWarn) const;
#endif

    template<typename T>
    bool getValueInternal(const YAML::Node& node, const string& name,
                          QList<T>& value, bool logWarn) const;

    template<typename T1, typename T2>
    bool getValueInternal(const YAML::Node& node, const string& name,
                          QPair<T1, T2>& value, bool logWarn) const;

    bool getValueInternal(const YAML::Node& node, const string& name,
                          QPoint& value, bool logWarn) const;

    bool getValueInternal(const YAML::Node& node, const string& name,
                          QSize& value, bool logWarn) const;
#endif

    template<typename T>
    static bool getValueProxy(const Config* config, YAML::Node& node,
                              const string& name, T& value, bool logWarn,
                              typename detail::is_yaml_type<T>::type = 0)
    {
        return config->getValueBase(node, name, value, logWarn);
    }
    template<typename T>
    static bool getValueProxy(const Config* config, YAML::Node& node,
                              const string& name, T& value, bool logWarn,
                              typename detail::not_yaml_type<T>::type = 0)
    {
        return config->getValueInternal(node, name, value, logWarn);
    }

private:
    template<typename T>
    bool setValueBase(YAML::Node& node, const string& name, const T& value);

    // Используется для записи значений для нод сложной конфигурации
    bool setValueInternal(YAML::Node& node, const string& name, Func func);

    template<typename VectorT>
    bool setValueVector(YAML::Node& node, const string& name,
                        const VectorT& value);

    template<typename T>
    bool setValueInternal(YAML::Node& node, const string& name,
                          const vector<T>& value);

#if defined(QT_CORE_LIB)

#if QT_VERSION < 0x060000
    template<typename T>
    bool setValueInternal(YAML::Node& node, const string& name,
                          const QVector<T>& value);
#endif

    template<typename T>
    bool setValueInternal(YAML::Node& node, const string& name,
                          const QList<T>& value);

    template<typename T1, typename T2>
    bool setValueInternal(YAML::Node& node, const string& name,
                          const QPair<T1, T2>& value);

    bool setValueInternal(YAML::Node& node, const string& name,
                          const QPoint& value);

    bool setValueInternal(YAML::Node& node, const string& name,
                          const QSize& value);
#endif

    template<typename T>
    static bool setValueProxy(Config* config, YAML::Node& node,
                              const string& name, const T& value,
                              typename detail::is_yaml_type<T>::type = 0)
    {
        return config->setValueBase(node, name, value);
    }
    template<typename T>
    static bool setValueProxy(Config* config, YAML::Node& node,
                              const string& name, const T& value,
                              typename detail::not_yaml_type<T>::type = 0)
    {
        return config->setValueInternal(node, name, value);
    }

private:
    atomic_bool _changed = {false};
    atomic_bool _readOnly = {false};
    atomic_bool _saveDisabled = {false};
    string _filePath;
    YAML::Node _root;

    mutable recursive_mutex _configLock;
    mutable atomic_int _lockedCount = {0};

    // Вспомогательная переменная, используется для вывода в лог информации
    // по не найденной ноде
    mutable string _nameNodeFunc;

    template<typename T, int> friend T& safe::singleton();
};

//------------------------------ Implementation ------------------------------

//template<typename T>
//inline char* Config::typeName()
//{
//#if defined(_MSC_VER)
//    return typeid(T).name();
//#else
//    return abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
//#endif
//}

#define YAML_GET_FUNC 0
#define YAML_SET_FUNC 1
#define YAML_RETURN(VAL) (VAL)

#define YAML_CONFIG_LOG_ERROR(ERROR, GETSET) \
    alog::Line logLine = \
    alog::logger().error(alog_line_location, "YamlConfig") \
        << "Yaml error"; \
    if (GETSET == YAML_GET_FUNC) \
        logLine << ". Failed to get parameter: " << (_nameNodeFunc + name); \
    else /*YAML_SET_FUNC*/ \
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
        if (_readOnly) { \
            alog::logger().warn(alog_line_location, "YamlConfig") \
                << "Failed to set parameter: " << name \
                << ". Config data is read only"; \
            return false; \
        }

#if defined(QT_CORE_LIB)
template<>
struct Config::ProxyType<QString>
{
    typedef string Type;
    static Type setter(const QString& s) {return Type(s.toUtf8().constData());}
    static QString getter(const Type& s) {return QString::fromUtf8(s.c_str());}
};
template<>
struct Config::ProxyType<QUuid>
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
struct Config::ProxyType<QUuidEx>
{
    typedef string Type;
    static Type setter(const QUuidEx& u) {return ProxyType<QUuid>::setter(u);}
    static QUuidEx getter(const Type& s) {return ProxyType<QUuid>::getter(s);}
};
#endif

template<typename T>
bool Config::getValue(const string& name, T& value, bool logWarn) const
{
    return getValue(_root, name, value, logWarn);
}

template<typename T>
bool Config::getValue(const YAML::Node& baseNode, const string& name, T& value,
                      bool logWarn) const
{
    Locker locker {this}; (void) locker;
    //lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeGet(baseNode, name, logWarn);
    if (!node || node.IsNull())
        return false;

    return getValueProxy(this, node, name, value, logWarn);
}

template<typename T>
bool Config::getValueBase(const YAML::Node& node, const string& name, T& value,
                          bool logWarn) const
{
    if (!node.IsScalar())
    {
        if (logWarn)
            alog::logger().warn(alog_line_location, "Config")
                << "Parameter " << (_nameNodeFunc + name) << " is not scalar type";
        return false;
    }

    YAML_CONFIG_TRY
    // было так: value = node.as<T>();
    value = ProxyType<T>::getter(node.as<typename ProxyType<T>::Type>());
    YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(false))
    return true;
}

template<typename VectorT>
bool Config::getValueVector(const YAML::Node& node, const string& name,
                            VectorT& value, bool logWarn) const
{
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
        v.push_back(ProxyType<ValueType>::getter(
                    n.as<typename ProxyType<ValueType>::Type>()));
        YAML_CONFIG_CATCH(YAML_GET_FUNC, YAML_RETURN(false))
    }
    value.swap(v);
    return true;
}

template<typename T>
bool Config::getValueInternal(const YAML::Node& node, const string& name,
                              vector<T>& value, bool logWarn) const
{
    return getValueVector(node, name, value, logWarn);
}

#if defined(QT_CORE_LIB)

#if QT_VERSION < 0x060000
template<typename T>
bool Config::getValueInternal(const YAML::Node& node, const string& name,
                              QVector<T>& value, bool logWarn) const
{
    return getValueVector(node, name, value, logWarn);
}
#endif

template<typename T>
bool Config::getValueInternal(const YAML::Node& node, const string& name,
                              QList<T>& value, bool logWarn) const
{
    return getValueVector(node, name, value, logWarn);
}

template<typename T1, typename T2>
bool Config::getValueInternal(const YAML::Node& node, const string& name,
                              QPair<T1, T2>& value, bool logWarn) const
{
    Config::Func func = [&value](Config* c, YAML::Node& n, bool logWarn)
    {
        T1 first; T2 second;
        bool r1 = c->getValue(n, "first",  first,  logWarn);
        bool r2 = c->getValue(n, "second", second, logWarn);
        if (r1 && r2)
            value = {first, second};

        return r1 && r2;
    };
    return getValueInternal(node, name, func, logWarn);
}
#endif

template<typename T>
bool Config::setValue(const string& name, const T& value)
{
    return setValue(_root, name, value);
}

template<typename T>
bool Config::setValue(YAML::Node& baseNode, const string& name, const T& value)
{
    YAML_CONFIG_CHECK_READONLY

    Locker locker {this}; (void) locker;
    //lock_guard<recursive_mutex> locker {_configLock}; (void) locker;

    YAML::Node node = nodeSet(baseNode, name);
    return setValueProxy(const_cast<Config*>(this), node, name, value);
}

template<typename T>
bool Config::setValueBase(YAML::Node& node, const string& name, const T& value)
{
    YAML_CONFIG_TRY
    node = YAML::Node(ProxyType<T>::setter(value));
    YAML_CONFIG_CATCH(YAML_SET_FUNC, YAML_RETURN(false))
    _changed = true;
    return true;
}

template<typename VectorT>
bool Config::setValueVector(YAML::Node& node, const string& name,
                            const VectorT& value)
{
    YAML_CONFIG_TRY
    node = YAML::Node();
    node.SetStyle(YAML::EmitterStyle::Flow);
    typedef typename VectorT::value_type ValueType;
    for (const ValueType& v : value)
        node.push_back(ProxyType<ValueType>::setter(v));
    YAML_CONFIG_CATCH(YAML_SET_FUNC, YAML_RETURN(false))
    _changed = true;
    return true;
}

template<typename T>
bool Config::setValueInternal(YAML::Node& node, const string& name,
                              const vector<T>& value)
{
    return setValueVector(node, name, value);
}

#if defined(QT_CORE_LIB)

#if QT_VERSION < 0x060000
template<typename T>
bool Config::setValueInternal(YAML::Node& node, const string& name,
                              const QVector<T>& value)
{
    return setValueVector(node, name, value);
}
#endif

template<typename T>
bool Config::setValueInternal(YAML::Node& node, const string& name,
                              const QList<T>& value)
{
    return setValueVector(node, name, value);
}

template<typename T1, typename T2>
bool Config::setValueInternal(YAML::Node& node, const string& name,
                              const QPair<T1, T2>& value)
{
    Config::Func func = [&value] (Config* c, YAML::Node& n, bool /*logWarn*/)
    {
        n.SetStyle(YAML::EmitterStyle::Flow);
        bool r1 = c->setValue(n, "first",  value.first);
        bool r2 = c->setValue(n, "second", value.second);
        return r1 && r2;
    };
    return setValueInternal(node, name, func);
}
#endif

// Вспомогательная функция, используется для последовательного чтения параметров
// из узлов node, baseNode.
template<typename T>
bool getValue(Config* config, const YAML::Node& node, const YAML::Node& baseNode,
              const string& name, T& value)
{
    // Читаем параметр из основного узла
    if (config->getValue(node, name, value, false))
        return true;

    // Если параметра нет в основном узле, пробуем прочитать базовый узел
    if (config->getValue(baseNode, name, value, false))
        return true;

    // Если параметр не найден, то вновь читаем основной узел чтобы вывести
    // сообщение в лог
    T dummy; (void) dummy;
    config->getValue(node, name, dummy, true /*пишем в лог*/);

    return false;
}

} // namespace yaml

typedef yaml::Config YamlConfig;
