/* clang-format off */
/*****************************************************************************
  В модуле реализован механизм чтения конфигурационных файлов записанных
  в YAML нотации.
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
#include <map>
#include <functional>
#if !defined(_MSC_VER)
#include <cxxabi.h>
#endif

#if defined(QT_CORE_LIB)
#include <QString>
#include <QVector>
#endif


class YamlConfig
{
public:
    typedef std::function<bool (YAML::Node&, bool)> Func;

    YamlConfig() = default;

    // Читает файл конфигурации. filePath - полное имя файла.
    bool read(const std::string& filePath);

    // Перечитывает файл конфигурации
    bool reRead();

    // Признак, что конфигурация используется только для чтения,
    // запись запрещена.
    bool readOnly() const;
    void setReadOnly(bool);

    // Сохраняет данные в файл
    bool save(const std::string& filePath = std::string());

    // Возвращает полное имя файла конфигурации
    std::string filePath() const;

    // Удаляет ноду по имени
    bool remove(const std::string& name, bool logWarnings = true);

    // Используется для получения простого значения (скаляр) из ноды
    // с именем name. Имя может быть составным. Составное имя записывается
    // следующим образом: 'param1.param2.param3'.
    // Возвращает TRUE если нода с именем 'name' существует и в параметр value
    // было успешно записано значение. Если параметр logWarnings равен TRUE,
    // то в случае неудачного считывания данных лог выводится сообщение
    // о причинах.
    template<typename T>
    bool getValue(const std::string& name,
                  T& value, bool logWarnings = true) const;

    // Используется для получения списка значений из ноды с именем name.
    // Возвращает TRUE если нода с именем 'name' существует и в параметр value
    // был успешно заполнен значениями. Если параметр logWarnings равен TRUE,
    // то в случае неудачного считывания данных лог выводится сообщение
    // о причинах.
    template<typename T>
    bool getValue(const std::string& name,
                  std::vector<T>& value, bool logWarnings = true) const;

#if defined(QT_CORE_LIB)
    // Перегруженная функция, используется для считывания списка значений
    // в QVector.
    template<typename T>
    bool getValue(const std::string& name,
                  QVector<T>& value, bool logWarnings = true) const;
#endif

    // Используется для получения значений для нод сложной конфигурации.
    // В качестве параметра func используется функция или функтор со следующей
    // сигнатурой function(YAML::Node& node, bool logWarnings), где параметр
    // node - это нода по имени name, а параметр logWarnings определяет
    // нужно ли выводить сообщения в лог в случае неудачного считывания данных.
    bool getValue(const std::string& name,
                  Func func, bool logWarnings = true) const;

    // Используется для записи простого значения (скаляр) в ноду с именем name.
    // Имя может быть составным. Составное имя записывается следующим образом:
    // 'param1.param2.param3'.
    // Возвращает TRUE если значение было удачно записано в ноду.
    template<typename T>
    bool setValue(const std::string& name, const T& value);

    // Используется для записи списка значений в ноду с именем name.
    // Возвращает TRUE если список был удачно записан в ноду.
    template<typename T>
    bool setValue(const std::string& name, const std::vector<T>& value);

#if defined(QT_CORE_LIB)
    // Перегруженная функция, используется для записи списка значений
    // из QVector.
    template<typename T>
    bool setValue(const std::string& name, const QVector<T>& value);
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

    // Используется в функциях getValue(). Возвращает ноду по имени 'name'.
    // Если ноды с заданным именем нет в списке - возвращает пустую ноду.
    YAML::Node nodeGetValue(const std::string& name, bool logWarnings = true) const;

    // Используется в функциях setValue(). Строит иерархию нод согласно задан-
    // ному параметру 'name'.
    YAML::Node nodeSetValue(const std::string& name);

    bool getValue(const std::string& name,
                  YAML::Node&, bool logWarnings = true) const;

    template<typename VectorT>
    bool getValueVect(const std::string& name,
                      VectorT& value, bool logWarnings = true) const;

    template<typename VectorT>
    bool setValueVect(const std::string& name, const VectorT& value);

private:
    std::atomic_bool _readOnly = {false};
    std::string _filePath;
    YAML::Node _root;
    mutable std::mutex _configLock;

    template<typename T, int> friend T& ::safe_singleton();
};

//---------------------------- Implementation  -------------------------------

template<typename T>
char* YamlConfig::typeName()
{
#if defined(_MSC_VER)
    return typeid(T).name();
#else
    return abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
}

#define YAMLCONFIG_LOG_ERROR(ERROR, GETSET) \
    alog::Line logLine = \
    alog::logger().error_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig") \
        << "Yaml error"; \
    if (GETSET == 0) \
        logLine << ". Failed to get parameter: " << name; \
    else \
        logLine << ". Failed to set parameter: " << name; \
    logLine << ". Detail: " << ERROR \
            << ". File: " << _filePath;

#define YAMLCONFIG_TRY \
    try {

#define YAMLCONFIG_CATCH(GETSET, RETURN) \
    } catch (YAML::Exception& e) { \
        YAMLCONFIG_LOG_ERROR(e.what(), GETSET) \
        return RETURN; \
    } catch (std::exception& e) { \
        YAMLCONFIG_LOG_ERROR(e.what(), GETSET) \
        return RETURN; \
    } catch (...) { \
        YAMLCONFIG_LOG_ERROR("Unknown error", GETSET) \
        return RETURN; \
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
bool YamlConfig::getValue(const std::string& name, T& value, bool logWarnings) const
{
    std::lock_guard<std::mutex> locker(_configLock); (void) locker;

    YAML::Node node;
    if (!getValue(name, node, logWarnings))
            return false;

    if (!node.IsScalar())
    {
        if (logWarnings)
            alog::logger().warn_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig")
                << "Parameter " << name << " is not scalar type";
        return false;
    }

    YAMLCONFIG_TRY
    // было так: value = node.as<T>();
    value = ProxyStdString<T>::getter(node.as<typename ProxyStdString<T>::Type>());
    YAMLCONFIG_CATCH(0, false)
    return true;
}

template<typename VectorT>
bool YamlConfig::getValueVect(const std::string& name,
                              VectorT& value, bool logWarnings) const
{
    YAML::Node node;
    if (!getValue(name, node, logWarnings))
            return false;

    if (!node.IsSequence())
    {
        if (logWarnings)
            alog::logger().warn_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig")
                << "Parameter " << name << " is not sequence type";
        return false;
    }

    VectorT v;
    for (const YAML::Node& n: node)
    {
        if (!n.IsScalar())
        {
            if (logWarnings)
                alog::logger().warn_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "YamlConfig")
                    << "Parameter " << name
                    << ". The elements of sequence are not a scalar type";
            return false;
        }
        YAMLCONFIG_TRY
        typedef typename VectorT::value_type ValueType;
        // было так: v.push_back(n.as<ValueType>());
        v.push_back(ProxyStdString<ValueType>::getter(
                        n.as<typename ProxyStdString<ValueType>::Type>()));
        YAMLCONFIG_CATCH(0, false)
    }
    value.swap(v);
    return true;
}

template<typename T>
bool YamlConfig::getValue(const std::string& name,
                          std::vector<T>& value, bool logWarnings) const
{
    std::lock_guard<std::mutex> locker(_configLock); (void) locker;
    return getValueVect(name, value, logWarnings);
}

#if defined(QT_CORE_LIB)
template<typename T>
bool YamlConfig::getValue(const std::string& name,
                          QVector<T>& value, bool logWarnings) const
{
    std::lock_guard<std::mutex> locker(_configLock); (void) locker;
    return getValueVect(name, value, logWarnings);
}
#endif

template<typename T>
bool YamlConfig::setValue(const std::string& name, const T& value)
{
    std::lock_guard<std::mutex> locker(_configLock); (void) locker;

    YAML::Node node = nodeSetValue(name);
    YAMLCONFIG_TRY
    node = YAML::Node(ProxyStdString<T>::setter(value));
    YAMLCONFIG_CATCH(1, false)
    return true;
}

template<typename VectorT>
bool YamlConfig::setValueVect(const std::string& name, const VectorT& value)
{
    YAML::Node node = nodeSetValue(name);
    YAMLCONFIG_TRY
    node = YAML::Node();
    typedef typename VectorT::value_type ValueType;
    for (const ValueType& v : value)
        node.push_back(ProxyStdString<ValueType>::setter(v));
    YAMLCONFIG_CATCH(1, false)
    return true;
}

template<typename T>
bool YamlConfig::setValue(const std::string& name, const std::vector<T>& value)
{
    std::lock_guard<std::mutex> locker(_configLock); (void) locker;
    return setValueVect(name, value);
}

#if defined(QT_CORE_LIB)
template<typename T>
bool YamlConfig::setValue(const std::string& name, const QVector<T>& value)
{
    std::lock_guard<std::mutex> locker(_configLock); (void) locker;
    return setValueVect(name, value);
}
#endif

