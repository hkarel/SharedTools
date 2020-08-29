/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2013 Pavel Karelin (hkarel), <hkarel@yandex.ru>
  Author of idea: Mikhail Evdokimov

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

  Реализация высоконагруженного асинхронного логгера.

*****************************************************************************/

#pragma once

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif

#include "list.h"
#include "clife_base.h"
#include "clife_ptr.h"
#include "simple_ptr.h"
#include "safe_singleton.h"
#include "thread/thread_base.h"
#include "thread/thread_utils.h"

#include <sys/types.h>
#include <sys/time.h>
#include <iostream>
#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <set>
#include <map>
#include <type_traits>

namespace alog /*async logger*/ {

using namespace std;

class Saver;
class Logger;

// Уровни log-сообщений
enum Level
{
    None    = 0,
    Error   = 1,
    Warning = 2,
    Info    = 3,
    Verbose = 4,
    Debug   = 5,
    Debug2  = 6
};

// Вспомогательная функция, используется для преобразования строкового
// обозначения уровня логирования в enum Level{}
Level levelFromString(const string& level);

// Вспомогательная функция, используется для получения строкового представления
// уровня логирования
string levelToString(Level);

/**
  Абстрактная структура, используется для передачи произвольных данных от точки
  логирования до сэйвера
*/
struct Something
{
    typedef simple_ptr<Something> Ptr;
    virtual ~Something() {}

    // Возвращает TRUE если есть необходимость в модификации сообщения
    // перед тем, как оно будет записано сэйвером
    virtual bool canModifyMessage() const;

    // Модифицирует сообщение перед тем, как оно будет записано сэйвером
    virtual string modifyMessage(const string&) const;
};

/**
  Базовое сообщение
*/
struct Message //: public clife_base
{
    Level  level;
    string str;
    string module;

    // Буферы prefix и prefix2 хранят результаты функций prefixFormatter{2}.
    // Основное назначение минимизировать количество вызовов prefixFormatter{2}
    // при записи сообщения сразу в несколько сэйверов.
    // Причина: большое потребление системных ресурсов при многократном вызове
    //          функций prefixFormatter{2}.
    // Важно:   prefix-буферы специально сделаны выделяемыми на стеке,
    //          так как память для них должна выделяться в момент создания
    //          объекта, а не в момент заполнения префиксов.
    char prefix [30]  = {0};
    char prefix2[10]  = {0};
    char prefix3[300] = {0};

    string file;
    string func;
    int    line;

    timeval timeVal;
    pid_t   threadId;

    Something::Ptr something;

    Message() {}
    Message(Message&&) = default;
    Message(const Message&) = delete;
    Message& operator= (Message&&) = delete;
    Message& operator= (const Message&) = delete;

};
typedef lst::List<Message, lst::CompareItemDummy> MessageList;
typedef simple_ptr<Message> MessagePtr;

/**
  Аллокатор используется для управления жизнью объектов Filter и Saver
*/
template<typename T>
struct AllocatorItem
{
    void destroy(T* x) {if (x) x->release();}
};

/**
  Функтор поиска, используется для поиска объектов Filter и Saver
*/
template<typename T>
struct FindItem
{
    int operator() (const string* name, const T* item2, void*) const
        {return name->compare(item2->name());}
};

/**
  Базовый класс механизма фильтрации

  Для того чтобы  механизм  фильтрации  мог  работать  в нескольких  потоках без
  дополнительных блокировок в Filter предусмотрен механизм запирания.  Суть  его
  сводится к следующему: сразу после создания  экземпляра  Filter - он  является
  не запертым. В это время ему можно назначить необходимые критерии  фильтрации.
  После того как параметры фильтрации сконфигурированы фильтр запирается. Теперь
  фильтр может использоваться  в механизмах  фильтрации,  но  изменить  критерии
  фильтрации для него уже нельзя. Если для запертого фильтра необходимо изменить
  критерии фильтрации,  то необходимо  выполнить следующие шаги:  создать  новый
  фильтр,  сконфигурировать его соответствующим образом,  запереть новый фильтр,
  и заменить старый фильтр новым
*/
class Filter : public clife_base
{
public:
    // Режим работы фильта: включающий/исключающий
    enum class Mode {Include, Exclude};

    Filter() {}
    virtual ~Filter() {}

    // Имя фильтра
    const string& name() const {return _name;}
    void setName(const string&);

    // Возвращает режим в котором работает фильтр: включающий/исключающий
    Mode mode() const {return _mode;}
    void setMode(Mode);

    // Определяет будут ли сообщения об ошибках фильтроваться так же, как и все
    // остальные сообщения. По умолчанию сообщения об ошибках не фильтруются
    bool filteringErrors() const {return _filteringErrors;}
    void setFilteringErrors(bool val);

    // Определяет будут ли в лог-файл включены  дополнительные  сообщения
    // которые находятся в одном и том же потоке с основными фильтруемыми
    // сообщениями.
    // Рассмотрим пример: имеется программный модуль  ImportData и по нему
    // выполняется фильтрация сообщений. При этом модуль ImportData активно
    // использует sql-запросы, которые отрабатывают в модуле SqlQuery.
    // Если просто включить модуль SqlQuery в фильтр, то будут выводиться
    // все запросы выполняемые в программе,  что  создаст  неудобства при
    // диагностике работы модуля ImportData. Параметр followThreadContext
    // позволит выводить только те sql-запросы, которые относятся к работе
    // модуля ImportData.
    // Особенности работы  механизма:  для фильтра  работающего  в режиме
    // Mode::Include сообщения идущие в контексте потока будут выводиться
    // в лог-файл; для фильтра Mode::Exclude сообщения идущие в контексте
    // потока не будут выводиться в лог-файл.
    // По умолчанию сообщения по контексту потока не фильтруются
    virtual bool followThreadContext() const;
    void setFollowThreadContext(bool val);

    enum class Check
    {
        NoLock    = -1, // Фильтр не заперт
        Fail      =  0, // Сообщение не соответствует критериям фильтрации
        Success   =  1, // Сообщение соответствует критериям фильтрации
        MessError =  2  // Сообщение является сообщением об ошибке
    };

    // Проверяет сообщение на соответствие критериям фильтрации
    Check check(const Message&) const;

    // Возвращает статус фильта: заперт/не заперт
    bool locked() const {return _locked;}

    // Запирает фильтр.
    void lock() {_locked = true;}

private:
    Filter(Filter&&) = default;
    Filter(const Filter&) = delete;
    Filter& operator= (Filter&&) = delete;
    Filter& operator= (const Filter&) = delete;

    virtual bool checkImpl(const Message&) const = 0;

    // Удаляет идентификаторы потоков из списка _threadContextIds. Удаление
    // происходит по истечении временного интервала в 3 секунды
    void removeIdsTimeoutThreads();

private:
    string _name;
    Mode   _mode = {Mode::Include};
    bool   _locked = {false};
    bool   _filteringErrors = {false};
    bool   _followThreadContext = {false};

    // Список идентификаторов потоков, используется для фильтрации сообщений
    // по контексту потока
    mutable map<pid_t, timeval> _threadContextIds;

    friend class Saver;
};
typedef lst::List<Filter, FindItem<Filter>, AllocatorItem<Filter>> FilterList;
typedef clife_ptr<Filter> FilterPtr;

/**
  Фильтр по именам модулей
*/
class FilterModule : public Filter
{
public:
    const set<string>& modules() const {return _modules;}

    // Добавляет модули на которые будет распространяться действие этого фильтра
    void addModule(const string& name);

    // Определяет будут ли неименованные модули обрабатываться данным фильтром.
    // По умолчанию неименованные модули не фильтруются
    bool filteringNoNameModules() const {return _filteringNoNameModules;}
    void setFilteringNoNameModules(bool val);

private:
    bool checkImpl(const Message&) const override;
    set<string> _modules;
    bool _filteringNoNameModules = {false};
};
typedef clife_ptr<FilterModule> FilterModulePtr;

/**
  Фильтр по уровню логирования
*/
class FilterLevel : public FilterModule
{
public:
    Level leve() const {return _level;}
    void setLevel(Level);

private:
    bool checkImpl(const Message&) const override;
    Level _level = {None};
};
typedef clife_ptr<FilterLevel> FilterLevelPtr;

/**
  Фильтр по именам файлов
*/
class FilterFile : public Filter
{
public:
    const set<string>& files() const {return _files;}

    // Добавляет файлы на которые будет распространяться действие этого фильтра
    void addFile(const string& name);

private:
    bool checkImpl(const Message&) const override;
    set<string> _files;
};
typedef clife_ptr<FilterFile> FilterFilePtr;

/**
  Фильтр по именам функций
*/
class FilterFunc : public Filter
{
public:
    const set<string>& funcs() const {return _funcs;}

    // Добавляет функции на которые будет распространяться действие этого фильтра
    void addFunc(const string& name);

private:
    bool checkImpl(const Message&) const override;
    set<string> _funcs;
};
typedef clife_ptr<FilterFunc> FilterFuncPtr;

/**
  Фильтр по идентификаторам потока
*/
class FilterThread : public Filter
{
public:
    bool followThreadContext() const override;
    const set<pid_t>& threads() const {return _threads;}

    // Добавляет функции на которые будет распространяться действие этого фильтра
    void addThread(long id);

private:
    bool checkImpl(const Message&) const override;
    set<pid_t> _threads;
};
typedef clife_ptr<FilterThread> FilterThreadPtr;

/**
  Фильтр по содержимому лог-сообщения
*/
class FilterContent : public Filter
{
public:
    const set<string>& contents() const {return _contents;}

    // Добавляет элемент контента на который будет распространяться действие
    // этого фильтра
    void addContent(const string& cont);

private:
    bool checkImpl(const Message&) const override;
    set<string> _contents;
};
typedef clife_ptr<FilterContent> FilterContentPtr;

/**
  Базовый класс механизма сохранения
*/
class Saver : public clife_base
{
public:
    Saver(const string& name, Level level = Error);
    virtual ~Saver() {}

    // Имя сэйвера
    const string& name() const {return _name;}

    // В основном используется при конфигурировании логгера, для указания
    // в конфиг-файле, что сэйвер является неактивным. При неактивном сэйвере
    // запись в лог-файл не производится
    bool active() const {return _active;}
    void setActive(bool val);

    // Уровень логирования
    Level level() const {return _level;}
    void  setLevel(Level val) {_level = val;}

    // Устанавливает ограничение на максимальную длину строки сообщения.
    // Длина строки не ограничивается если значение меньше либо равно 0.
    // Значение по умолчанию 5000.
    // TODO: Для utf8 кодировки учесть, чтобы обрезка не происходила
    //       посередине символов
    int  maxLineSize() const {return _maxLineSize;}
    void setMaxLineSize(int val) {_maxLineSize = val;}

    Logger* logger() const {return _logger;}
    void setLogger(Logger* val) {_logger = val;}

    // Выполняет запись буфера сообщений
    void flush(const MessageList&);

    // Возвращает snapshot-список фильтров
    FilterList filters() const;

    // Добавляет фильтр в список фильтров. Если фильтр с указанным именем уже
    // существует, то он будет заменен новым
    void addFilter(FilterPtr);

    // Признак активности системы фильтрации
    bool filtersActive() const {return _filtersActive;}
    void setFiltersActive(bool val) {_filtersActive.store(val);}

    // Удаляет фильтр
    void removeFilter(const string& name);

    // Очищает список фильтров
    void clearFilters();

protected:
    virtual void flushImpl(const MessageList&) = 0;

    // Определяет будет ли сообщение  выводиться  в лог-файл,  возвращает TRUE
    // если сообщение не удовлетворяет условиям фильтрации.
    // Порядок работы функции: сообщение m последовательно обрабатывается всеми
    // фильтрами filters. Если сообщение не удовлетворяет критериям фильтрации
    // очередного фильтра, то функция завершает работу с результатом TRUE
    bool skipMessage(const Message& m, const FilterList& filters);

    void removeIdsTimeoutThreads();

private:
    Saver() = delete;
    Saver(Saver&&) = delete;
    Saver(const Saver&) = delete;
    Saver& operator= (Saver&&) = delete;
    Saver& operator= (const Saver&) = delete;

private:
    string _name;
    bool   _active = {true};
    Level  _level = {Error};
    int    _maxLineSize = {5000};

    FilterList  _filters;
    atomic_bool _filtersActive = {true};
    mutable atomic_flag _filtersLock = ATOMIC_FLAG_INIT;

    atomic<Logger*> _logger = {0};

    friend class SaverStdOut;
    friend class SaverStdErr;
};
typedef lst::List<Saver, FindItem<Saver>, AllocatorItem<Saver>> SaverList;
typedef clife_ptr<Saver> SaverPtr;

/**
  Вывод в stdout
*/
class SaverStdOut : public Saver
{

public:
    // Если параметр shortMessages == TRUE,  то в консоль будут выводятся только
    // сами сообщения, а расширенные параметры сообщения такие как дата, уровень
    // логирования, идентификатор потока и пр. выводиться не будут
    SaverStdOut(const char* name, Level level, bool shortMessages);
    void flushImpl(const MessageList&) override;

protected:
    ostream* _out;
    bool _shortMessages = {false};
};

/**
  Вывод в stderr
*/
class SaverStdErr : public SaverStdOut
{
public:
    SaverStdErr(const char* name, Level level, bool shortMessages);
};

/**
  Вывод в файл
*/
class SaverFile : public Saver
{
public:
    SaverFile(const string& name,
              const string& filePath,
              Level level = Error,
              bool isContinue = true);
    void flushImpl(const MessageList&) override;

    // Возвращает полный путь до лог-файла
    string filePath() const {return _filePath;}

    // Если параметр установлен в TRUE, то запись данных будет продолжена
    // в существующий лог-файл, в противном случае лог-файл будет очищен
    // при создании сэйвера
    bool isContinue() const {return _isContinue;}

private:
    string _filePath;
    bool   _isContinue = {true};
};
typedef clife_ptr<SaverFile> SaverFilePtr;

/**
  Базовая структура, используется для формирования строки вида:
  logger().debug << "test" << 123;
*/
struct Line
{
    Line(Logger*     logger,
         Level       level,
         const char* file,
         const char* func,
         int         line,
         const char* module);

    // В деструкторе происходит окончательное формирование сообщения,
    // и добавление его в коллекцию сообщений логгера
    ~Line();

    Line(Line&&) = default;

    Line() = delete;
    Line(const Line&) = delete;
    Line& operator= (Line&&) = delete;
    Line& operator= (const Line&) = delete;

    // Сервисная функция, выполняет проверку уровня логирования и определяет
    // нужно ли добавлять сообщение в логгер
    bool toLogger() const;

    struct Impl
    {
        Logger*        logger;
        Level          level;
        const char*    file;   // Наименование файла
        const char*    func;   // Наименование функции
        int            line;   // Номер строки вызова
        const char*    module; // Наименование модуля
        string         buff;
        Something::Ptr something; // Параметр используется  для  передачи
                                  // произвольных данных от точки логгиро-
                                  // вания до сэйвера
    };
    simple_ptr<Impl> impl;
};

/**
  Logger
*/
class Logger : public trd::ThreadBase
{
public:
    ~Logger();

    Line error  (const char* file, const char* func, int line, const char* module = 0);
    Line warn   (const char* file, const char* func, int line, const char* module = 0);
    Line info   (const char* file, const char* func, int line, const char* module = 0);
    Line verbose(const char* file, const char* func, int line, const char* module = 0);
    Line debug  (const char* file, const char* func, int line, const char* module = 0);
    Line debug2 (const char* file, const char* func, int line, const char* module = 0);

    // Записывает все сообщения буфера. Парамет loop определяет сколько циклов
    // сброса данных нужно сделать чтобы все сообщения очереди были  записаны.
    // В большинстве случаев достаточно одного цикла,  но когда  лог-сообщения
    // поступают  очень интенсивно - целесообразно  этот  параметр  установить
    // в 2 или 3
    void flush(int loop = 1);

    // Заставляет вызывающий поток ждать, пока все сообщения буфера будут
    // записаны в лог-файлы
    void waitingFlush();

    // Добавляет сэйвер для вывода лог-сообщений в stdout. Если сэйвер уже был
    // добавлен ранее, то сэйвер будет пересоздан с новым уровнем логирования.
    // Если параметр shortMessages == TRUE, то в консоль будут выводятся только
    // сами сообщения, а расширенные параметры сообщения такие как дата, уровень
    // логирования, идентификатор потока и пр. выводиться не будут
    void addSaverStdOut(Level level = Error, bool shortMessages = false);

    // Добавляет сэйвер для вывода лог-сообщений в stderr. Если сэйвер уже был
    // добавлен ранее, то сэйвер будет пересоздан с новым уровнем логирования
    void addSaverStdErr(Level level = Error, bool shortMessages = false);

    // Удаляет сэйвер выводящий лог-сообщения в stdout
    void removeSaverStdOut();

    // Удаляет сэйвер выводящий лог-сообщения в stderr
    void removeSaverStdErr();

    // Включает вывод информации в логи (по умолчанию включено)
    void on() NOEXCEPT {_on = true;}

    // Позволяет временно отключить вывод данных в логи
    void off() NOEXCEPT {_on = false;}

    // Определяет интервал записи сообщений для сэйверов.
    // Измеряется в миллисекундах, значение по умолчанию 300 ms
    int  flushTime() const {return _flushTime;}
    void setFlushTime(int val) {_flushTime = val;}

    // Определяет размер буфера после которого будет выполнена запись сообщений
    // для CUSTOM-сэйверов. Значение по умолчанию 1000
    int  flushSize() const {return _flushSize;}
    void setFlushSize(int val) {_flushSize = val;}

    // Добавляет сэйвер в список сэйверов. Если сэйвер с указанным именем уже
    // существует, то он будет заменен новым
    void addSaver(SaverPtr);

    // Удаляет сэйвер
    void removeSaver(const string& name);

    // Выполняет поиск сэйвера по имени
    SaverPtr findSaver(const string& name);

    // Очищает список сэйверов
    void clearSavers(bool clearStd = true);

    // Возвращает snapshot сэйверов
    SaverList savers(bool withStd = true) const;

    // Возвращает максимальный уровень логирования для сэйверов зарегистрированных
    // на данный момент в логгере
    Level level() const {return _level;}

    void redefineLevel();

private:
    Logger();
    Logger(Logger&&) = delete;
    Logger(const Logger&) = delete;
    Logger& operator= (Logger&&) = delete;
    Logger& operator= (const Logger&) = delete;

    void addMessage(MessagePtr&&);
    void run() override;

private:
    MessageList _messages;
    mutable atomic_flag  _messagesLock = ATOMIC_FLAG_INIT;

    SaverPtr  _saverOut;  // Сэйвер для STDOUT
    SaverPtr  _saverErr;  // Сэйвер для STDERR
    SaverList _savers;    // Список CUSTOM-сэйверов
    mutable atomic_flag  _saversLock = ATOMIC_FLAG_INIT;

    volatile Level _level = {None};

    int _flushTime = {300};
    int _flushSize = {1000};
    volatile int _flushLoop = {0};
    volatile bool _on = {true};

    friend struct Line;
    template<typename T, int> friend T& ::safe_singleton();
};

Logger& logger();

//---------------------------------- Logger ----------------------------------

inline Line Logger::error(const char* file, const char* func, int line, const char* module)
{
    return Line(this, Error, file, func, line, module);
}

inline Line Logger::warn(const char* file, const char* func, int line, const char* module)
{
    return Line(this, Warning, file, func, line, module);
}

inline Line Logger::info(const char* file, const char* func, int line, const char* module)
{
    return Line(this, Info, file, func, line, module);
}

inline Line Logger::verbose(const char* file, const char* func, int line, const char* module)
{
    return Line(this, Verbose, file, func, line, module);
}

inline Line Logger::debug(const char* file, const char* func, int line, const char* module)
{
    return Line(this, Debug, file, func, line, module);
}

inline Line Logger::debug2(const char* file, const char* func, int line, const char* module)
{
    return Line(this, Debug2, file, func, line, module);
}

//------------------------------ Line operators ------------------------------

inline bool Line::toLogger() const
{
    return (impl) ? (impl->level <= impl->logger->level()) : false;
}

Line& operator<< (Line&, bool);
Line& operator<< (Line&, char);
Line& operator<< (Line&, char*);
Line& operator<< (Line&, const char*);
Line& operator<< (Line&, const string&);
Line& operator<< (Line&, const timeval&);

namespace detail {

template<typename T>
using is_arithmetic = std::enable_if<std::is_arithmetic<T>::value, int>;

template <typename T>
using is_enum_type = std::enable_if<std::is_enum<T>::value, int>;

template<typename T>
using not_supported = std::enable_if<!std::is_arithmetic<T>::value
                                  && !std::is_enum<T>::value, int>;

template<typename T>
Line& stream_operator(Line& line, const T t, typename is_arithmetic<T>::type = 0)
{
    if (line.toLogger())
        line.impl->buff += std::to_string(t);
    return line;
}

template<typename T>
Line& stream_operator(Line& line, const T t, typename is_enum_type<T>::type = 0)
{
    if (line.toLogger())
    {
        typedef typename std::underlying_type<T>::type enum_type;
        line.impl->buff += std::to_string(static_cast<enum_type>(t));
    }
    return line;
}

template<typename T>
Line& stream_operator(Line& line, const T, typename not_supported<T>::type = 0)
{
    static_assert(!std::is_same<typename not_supported<T>::type, int>::value,
                  "Unknown type for stream operator");
    return line;
}

} // namespace detail

template<typename T>
Line& operator<< (Line& line, const T& t)
{
    return detail::stream_operator(line, t);
}

template<typename T>
Line operator<< (Line&& line, const T& t)
{
    operator<< (line, t);
    return std::move(line);
}

} // namespace alog

#define log_error   alog::logger().error  (__FILE__, __func__, __LINE__)
#define log_warn    alog::logger().warn   (__FILE__, __func__, __LINE__)
#define log_info    alog::logger().info   (__FILE__, __func__, __LINE__)
#define log_verbose alog::logger().verbose(__FILE__, __func__, __LINE__)
#define log_debug   alog::logger().debug  (__FILE__, __func__, __LINE__)
#define log_debug2  alog::logger().debug2 (__FILE__, __func__, __LINE__)
