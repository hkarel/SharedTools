/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2020 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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
  ---

  Модуль содержит функции по работе с конфигурационными файлами приложения.

*****************************************************************************/

#pragma once

#include "defmac.h"
#include "yaml_config.h"
#include "safe_singleton.h"

#include <ctime>
#include <string>

#ifdef QT_CORE_LIB
#include <QtCore>
#endif
#ifdef QT_NETWORK_LIB
#include <QHostAddress>
#endif

namespace config {

using namespace std;

// Возвращает путь до директории с файлами конфигурации для проекта
string dir();

// Развертывает имя файла конфигурации до полного пути
string path(const string& configFile);

#ifdef QT_CORE_LIB
QString qdir();
QString qpath(const QString& configFile);
#endif

// Базовые настройки приложения
YamlConfig& base();

// Сохраняемые настройки приложения
YamlConfig& state();

// Рабочие настройки приложения. Данный конфиг может относиться, как к базовым
// настройкам приложения, так и к сохраняемым
YamlConfig& work();

// Выполняет расширение пути для следующих случаев:
//   1) Символ '~' до полного пути к домашней директории;
//   2) Слово 'ProgramData' до директории расположения данных программы;
//   3) Слово 'AppData' до директории расположения настроек/данных программы.
// Пункты 2, 3 актуальны только для Windows.
// Примеры:
//   ~/.config/my_appl   -> /home/my_user/.config/my_appl (для Linux)
//   ~/.config/my_appl   -> C:/Users/my_user/.config/my_appl (для Windows)
//   ProgramData/my_appl -> C:/ProgramData/my_appl
//   AppData/my_appl     -> C:/Users/my_user/AppData/Roaming/my_appl
void dirExpansion(string& filePath);

#ifdef QT_CORE_LIB
void dirExpansion(QString& filePath);
#endif

// Возвращает время модификации базового конфиг-файла
time_t baseModifyTime();

// Возвращает наименование конфиг-файла для логгера
string loggerConfFile();

// Возвращает время модификации конфиг-файла для логгера
time_t loggerModifyTime();

#ifdef QT_NETWORK_LIB
// Возвращает host-адрес из файла конфигурации
bool readHostAddress(const QString& confHostStr, QHostAddress&);
#endif

#ifdef QT_CORE_LIB
/**
  Проверяет факт модификации файла по изменению его  временной  метки.
  Примечание: Функции start() и stop() должны вызываться из основного
              потока приложения после сознания экземпляра QCoreApplication
*/
class Observer : public QObject
{
public:
    Observer();

    void start(int timeout = 15 /*сек*/);
    void stop();

    void addFile(const QString& filePath);
    void removeFile(const QString& filePath);

    // Список наблюдаемых файлов
    QStringList files() const;

    // Очищает список наблюдаемых файлов
    void clear();

    // Возвращает время последней модификации файла
    static time_t modifyTime(const QString& filePath);

signals:
    // Сигнал эмитируется когда любой из файлов был изменен
    void changed();

    // Сигнал эмитируется когда файл filePath был изменен
    void changedItem(const QString& filePath);

private slots:
    void timeout();

private:
    Q_OBJECT
    DISABLE_DEFAULT_COPY(Observer)

    QTimer _timer;

    //          Время последней модификации файла
    //          |       Полный путь до файла (filePath)
    //          |       |
    QList<QPair<time_t, QString>> _files;
    mutable QMutex _filesLock;

    template<typename T, int> friend T& safe::singleton();
};
Observer& observer();

/**
  Проверяет факт модификации базового конфиг-файла и файла конфигурации логгера.
  Механизм работает на основе класса Observer.
  Примечание: Функции start() и stop() должны вызываться  из  основного  потока
              приложения после сознания экземпляра QCoreApplication
*/
class ObserverBase : public QObject
{
public:
    void start(int timeout = 15 /*сек*/);
    void stop();

signals:
    void changed();

private slots:
    void changedItem(const QString& filePath);

private:
    Q_OBJECT
    ObserverBase();
    DISABLE_DEFAULT_COPY(ObserverBase)

    Observer _observer;

    template<typename T, int> friend T& safe::singleton();
};
ObserverBase& observerBase();

#endif // QT_CORE_LIB

namespace detail {

template<typename VectorT>
void checkSizeMetric(VectorT& values, const VectorT& defaultValues)
{
    if (defaultValues.size() < values.size())
    {
        values.resize(defaultValues.size());
        return;
    }
    if (defaultValues.size() > values.size())
    {
        auto it = defaultValues.begin() + values.size();
        for (; it != defaultValues.end(); ++it)
            values.push_back(*it);
    }
}

} // namespace detail

// Проверяет прочитанный из конфиг-файла список значений на соответствие списку
// по умолчанию. Если прочитанный список оказывается короче, то он будет допол-
// нен элементами из списка по умолчанию
template<typename T>
void checkSizeMetric(vector<T>& values, const vector<T>& defaultValues)
{
    detail::checkSizeMetric(values, defaultValues);
}

#ifdef QT_CORE_LIB
template<typename T>
void checkSizeMetric(QVector<T>& values, const QVector<T>& defaultValues)
{
    detail::checkSizeMetric(values, defaultValues);
}
#endif

} // namespace config
