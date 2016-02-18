/* clang-format off */
/*****************************************************************************
  Огиринальная идея логгера: Евдокимов Михаил
  Переработка: Карелин Павел

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

#include "_list.h"
#include "clife_base.h"
#include "clife_ptr.h"
#include "simple_ptr.h"
#include "safe_singleton.h"
#include "thread/thread_base.h"
#include "thread/thread_info.h"

#include <sys/types.h>
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <atomic>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <thread>
#include <mutex>
#include <string>


namespace alog /*async logger*/ {

using namespace std;

class Saver;
class Logger;

// Уровни log-сообщений
enum Level {NONE, ERROR, WARNING, INFO, VERBOSE, DEBUG, DEBUG2};

// Вспомогательная функция, используется для преобразования строкового
// обозначения уровня логирования в enum Level{}
Level levelFromString(const string& level);

// Вспомогательная функция, используется для получения строкового представления
// уровня логирования.
string levelToString(Level);


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
    //          prefixFormatter{2}.
    // Важно:   prefix-буферы специально сделаны выделяемыми на стеке,
    //          так как память для них должна выделяться в момент создания
    //          объекта, а не в момент заполнения префиксов.
    char prefix [30]  = {0};
    char prefix2[10]  = {0};
    char prefix3[300] = {0};

    string file;
    string func;
    int    line;

    struct timeval timeVal;
    pid_t  threadId;

    Message() {}
    Message(Message&&) = default;
    Message(const Message&) = delete;
    Message& operator= (Message&&) = delete;
    Message& operator= (const Message&) = delete;

};
typedef lst::List<Message, lst::CompareItemDummy> MessageList;
typedef simple_ptr<Message> MessagePtr;


/**
  Аллокатор используется для управления жизнью объектов Filter и Saver.
*/
template<typename T>
struct AllocatorItem
{
    void destroy(T* x) {if (x) x->release();}
};

/**
  Функтор поиска, используется для поиска объектов Filter и Saver.
*/
template<typename T>
struct FindItem
{
    int operator() (const string* name, const T* item2, void*) const
        {return name->compare(item2->name());}
};


/**
  Базовый класс механизма фильтрации.

  Для того чтобы механизм фильтрации мог работать в нескольких потоках без допол-
  нительных блокировок в Filter предусмотрен механизм запирания. Суть его сводится
  к следующему: сразу после создания экземпляра Filter - он является не запертым.
  В это время ему можно назначить необходимые критерии фильтрации. После того как
  параметры фильтрации сконфигурированы фильтр запирается. Теперь фильтр может
  использоваться в механизмах фильтрации, но изменить критерии фильтрации для него
  уже нельзя. Если для запертого фильтра необходимо изменить критерии фильтрации,
  то необходимо выполнить следующие шаги: создать новый фильтр, с сконфигурировать
  его соответствующим образом, запереть новый фильтр, и заменить старый фильтр
  новым.
*/
class Filter : public clife_base
{
public:
    // Режим работы фильта: включающий/исключающий.
    enum class Mode {Include, Exclude};

    Filter() {}
    virtual ~Filter() {}

    // Имя фильтра
    const string& name() const {return _name;}
    void setName(const string&);

    // Возвращает режим в котором работает фильтр: включающий/исключающий.
    Mode mode() const {return _mode;}
    void setMode(Mode);

    // Определяет будут ли сообщения об ошибках фильтроваться так же, как и все
    // остальные сообщения. По умолчанию сообщения об ошибках не фильтруются.
    bool filteringErrors() const {return _filteringErrors;}
    void setFilteringErrors(bool val);

    // Определяет будут ли в лог-файл включены дополнительные сообщения
    // которые находятся в одном и том же потоке с основными фильтруемыми
    // сообщениями.
    // Рассмотрим пример: имеется программный модуль ImportData и по нему
    // выполняется фильтрация сообщений. При этом модуль ImportData активно
    // использует sql-запросы, которые отрабатывают в модуле SqlQuery.
    // Если просто включить модуль SqlQuery в фильтр, то будут выводиться
    // все запросы выполняемые в программе, что создаст неудобства при
    // диагностике работы модуля ImportData. Параметр followThreadContext
    // позволит выводить только те sql-запросы, которые относятся к работе
    // модуля ImportData.
    // Особенности работы механизма: для фильтра работающего в режиме
    // Mode::Include сообщения идущие в контексте потока будут выводиться
    // в лог-файл; для фильтра Mode::Exclude сообщения идущие в контексте
    // потока не будут выводиться в лог-файл.
    // По умолчанию сообщения по контексту потока не фильтруются.
    virtual bool followThreadContext() const;
    void setFollowThreadContext(bool val);

    enum class Check
    {
        NoLock    = -1, // Фильтр не заперт
        Fail      =  0, // Сообщение не соответствует критериям фильтрации
        Success   =  1, // Сообщение соответствует критериям фильтрации
        MessError =  2  // Cообщение является сообщением об ошибке
    };

    // Проверяет сообщение на соответствие критериям фильтрации
    Check check(const Message&) const;

    // Возвращает статус фильта: заперт/не заперт.
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
    // происходит по истечении временного интервала в 3 секунды.
    void removeIdsTimeoutThreads();

private:
    string _name;
    Mode   _mode = {Mode::Include};
    bool   _locked = {false};
    bool   _filteringErrors = {false};
    bool   _followThreadContext = {false};

    // Список идентификаторов потоков, используется для фильтрации сообщений
    // по контексту потока.
    mutable map<pid_t, struct timeval> _threadContextIds;

    friend class Saver;
};
typedef lst::List<Filter, FindItem<Filter>, AllocatorItem<Filter>> FilterList;
typedef clife_ptr<Filter> FilterLPtr;


/**
  Фильтр по именам модулей
*/
class FilterModule : public virtual Filter
{
public:
    const set<string>& modules() const {return _modules;}

    // Добавляет модули на которые будет распространяться действие этого фильтра
    void addModule(const string& name);

    // Определяет будут ли неименованные модули обрабатываться данным фильтром.
    // По умолчанию неименованные модули не фильтруются.
    bool filteringNoNameModules() const {return _filteringNoNameModules;}
    void setFilteringNoNameModules(bool val);

private:
    bool checkImpl(const Message&) const override;
    set<string> _modules;
    bool _filteringNoNameModules = {false};
};
typedef clife_ptr<FilterModule> FilterModuleLPtr;


/**
  Фильтр по уровню логирования.
*/
class FilterLevel : public FilterModule
{
public:
    Level leve() const {return _level;}
    void setLevel(Level);

private:
    bool checkImpl(const Message&) const override;
    Level _level = {NONE};
};
typedef clife_ptr<FilterLevel> FilterLevelLPtr;


/**
  Фильтр по именам файлов
*/
class FilterFile : public virtual Filter
{
public:
    const set<string>& files() const {return _files;}

    // Добавляет файлы на которые будет распространяться действие этого фильтра
    void addFile(const string& name);

private:
    bool checkImpl(const Message&) const override;
    set<string> _files;
};
typedef clife_ptr<FilterFile> FilterFileLPtr;


/**
  Фильтр по именам функций
*/
class FilterFunc : public virtual Filter
{
public:
    const set<string>& funcs() const {return _funcs;}

    // Добавляет функции на которые будет распространяться действие этого фильтра
    void addFunc(const string& name);

private:
    bool checkImpl(const Message&) const override;
    set<string> _funcs;
};
typedef clife_ptr<FilterFunc> FilterFuncLPtr;


/**
  Фильтр по идентификаторам потока
*/
class FilterThread : public virtual Filter
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
typedef clife_ptr<FilterThread> FilterThreadLPtr;


/**
  Базовый класс механизма сохранения
*/
class Saver : public clife_base
{
public:
    Saver(const string& name, Level level = ERROR);
    virtual ~Saver() {}

    Saver() = delete;
    Saver(Saver&&) = default;
    Saver(const Saver&) = delete;
    Saver& operator= (Saver&&) = delete;
    Saver& operator= (const Saver&) = delete;

    // Имя сэйвера
    const string& name() const {return _name;}

    // В основном используется при конфигурировании логгера, для указания
    // в конфиг-файле, что сэйвер является неактивным. При неактивном сэйвере
    // запись в лог-файл не производится.
    bool active() const {return _active;}
    void setActive(bool val);

    // Уровень логирования
    Level level() const {return _level;}
    void  setLevel(Level val) {_level = val;}

    // Устанавливает ограничение на максимальную длину строки сообщения.
    // Длина строки не ограничивается если значение меньше либо равно 0.
    // Значение по умолчанию 5000.
    // TODO: Для utf8 кодировки учесть, чтобы обрезка не происходила посередине
    //       символов.
    int  maxLineSize() const {return _maxLineSize;}
    void setMaxLineSize(int val) {_maxLineSize = val;}

    Logger* logger() const {return _logger;}
    void setLogger(Logger* val) {_logger = val;}

    // Выполняет запись буфера сообщений
    void flush(const MessageList&);

    // Возвращает snapshot-список фильтров
    FilterList filters() const;

    // Добавляет фильтр в список фильтров. Если фильтр с указанным именем уже
    // существует, то он будет заменен новым.
    void addFilter(FilterLPtr);

    // Удаляет фильтр
    void removeFilter(const string& name);

    // Очищает список фильтров
    void clearFilters();

protected:
    virtual void flushImpl(const MessageList&) = 0;

    // Определяет будет ли сообщение выводиться в лог-файл, возвращает TRUE
    // если сообщение не удовлетворяет условиям фильтрации.
    // Порядок работы функции: сообщение m последовательно обрабатывается всеми
    // фильтрами filters. Если сообщение не удовлетворяет критериям фильтрации
    // очередного фильтра, то функция завершает работу с результатом TRUE.
    bool skipMessage(const Message& m, const FilterList& filters);

    void removeIdsTimeoutThreads();

private:
    string _name;
    bool   _active = {true};
    Level  _level = {ERROR};
    int    _maxLineSize = {5000};

    FilterList _filters;
    mutable atomic_flag  _filtersLock = ATOMIC_FLAG_INIT;

    atomic<Logger*> _logger = {0};

    friend class SaverStdOut;
    friend class SaverStdErr;
};
typedef lst::List<Saver, FindItem<Saver>, AllocatorItem<Saver>> SaverList;
typedef clife_ptr<Saver> SaverLPtr;


/**
  Вывод в stdout
*/
class SaverStdOut : public Saver
{

public:
    // Если параметр shortMessages == TRUE, то в консоль будут выводятся только
    // сами сообщения, а расширенные параметры сообщения такие как дата, уровень
    // логирования, идентификатор потока и пр. выводиться не будут.
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
    SaverFile(const string& name, const string& filePath, Level level = ERROR);
    void flushImpl(const MessageList&) override;

    // Возвращает полный путь до лог-файла
    string filePath() const {return _filePath;}

private:
    string _filePath;
};
typedef clife_ptr<SaverFile> SaverFileLPtr;


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

    Line() = default;
    Line(Line&&) = default;
    Line& operator= (Line&&) = default;

    Line(const Line&) = delete;
    Line& operator= (const Line&) = delete;

    // Сервисная функция, выполняет проверку уровня логирования и определяет
    // нужно ли добавлять сообщение в логгер.
    inline bool toLogger() const;

    struct Impl
    {
        Logger*       logger;
        Level         level;
        const char*   file;   // Наименование файла
        const char*   func;   // Наименование функции
        int           line;   // Номер строки вызова
        const char*   module; // Наименование модуля
        stringstream  buff;
    };
    simple_ptr<Impl> impl;
};

template<typename T>
Line& operator<< (Line& line, const T& t);

template<typename T>
Line operator<< (Line&& line, const T& t);


/**
  Вспомогательная структура, используется для формирования строки вида:
  logger().debug << "test" << 123;
*/
struct LevelProxy
{
    LevelProxy(Logger*     logger,
               Level       level,
               const char* file = 0,
               const char* func = 0,
               int         line = -1,
               const char* module = 0);

    template<typename T>
    Line operator<< (const T& t)
        {return std::move(Line(logger, level, file, func, line, module) << t);}

    Logger* const logger;
    Level         level;  // Уровень log-сообщения
    const char*   file;   // Наименование файла
    const char*   func;   // Наименование функции
    int           line;   // Номер строки вызова
    const char*   module; // Наименование модуля
};


/**
  Logger
*/
class Logger : public trd::ThreadBase
{
public:
    ~Logger();

    LevelProxy error   = LevelProxy(this, ERROR);
    LevelProxy warn    = LevelProxy(this, WARNING);
    LevelProxy info    = LevelProxy(this, INFO);
    LevelProxy verbose = LevelProxy(this, VERBOSE);
    LevelProxy debug   = LevelProxy(this, DEBUG);
    LevelProxy debug2  = LevelProxy(this, DEBUG2);

    LevelProxy error_f  (const char* file, const char* func, int line, const char* module = 0);
    LevelProxy warn_f   (const char* file, const char* func, int line, const char* module = 0);
    LevelProxy info_f   (const char* file, const char* func, int line, const char* module = 0);
    LevelProxy verbose_f(const char* file, const char* func, int line, const char* module = 0);
    LevelProxy debug_f  (const char* file, const char* func, int line, const char* module = 0);
    LevelProxy debug2_f (const char* file, const char* func, int line, const char* module = 0);

    // Записывает все сообщения буфера.
    // Параметр pauseDuration позволяет задать время (в миллисекундах) приоста-
    // новки вызывающего потока для того чтобы логгер успел записать буффер
    // сообщений. Явно указывать pauseDuration нужно только в экстренных случаях,
    // например перед критическим завершением программы.
    void flush();

    // Заставляет вызывающий поток ждать, пока все сообщения буфера будут
    // записаны в лог-файлы.
    void waitingFlush();

    // Добавляет сэйвер для вывода лог-сообщений в stdout. Если сэйвер уже был
    // добавлен ранее, то сэйвер будет пересоздан с новым уровнем логирования.
    // Если параметр shortMessages == TRUE, то в консоль будут выводятся только
    // сами сообщения, а расширенные параметры сообщения такие как дата, уровень
    // логирования, идентификатор потока и пр. выводиться не будут.
    void addSaverStdOut(Level level = ERROR, bool shortMessages = false);

    // Добавляет сэйвер для вывода лог-сообщений в stderr. Если сэйвер уже был
    // добавлен ранее, то сэйвер будет пересоздан с новым уровнем логирования.
    void addSaverStdErr(Level level = ERROR, bool shortMessages = false);

    // Удаляет сэйвер выводящий лог-сообщения в stdout
    void removeSaverStdOut();

    // Удаляет сэйвер выводящий лог-сообщения в stderr
    void removeSaverStdErr();

    // Включает вывод информации в логи (по умолчанию включено)
    void on() NOEXCEPT {_on = true;}

    // Позволяет верменно отключить вывод данных в логи
    void off() NOEXCEPT {_on = false;}

    // Определяет интерывал записи сообщений для сэйверов.
    // Измеряется в миллисекундах, значение по умолчанию 300 ms.
    int  flushTime() const {return _flushTime;}
    void setFlushTime(int val) {_flushTime = val;}

    // Определяет размер буфера после которого будет выполена запись сообщений
    // для CUSTOM-сэйверов. Значение по умолчанию 1000.
    int  flushSize() const {return _flushSize;}
    void setFlushSize(int val) {_flushSize = val;}

    // Добавляет сэйвер в список сэйверов. Если сэйвер с указанным именем уже
    // существует, то он будет заменен новым.
    void addSaver(SaverLPtr);

    // Удаляет сэйвер.
    void removeSaver(const string& name);

    // Выполняет поиск сэйвера по имени.
    SaverLPtr findSaver(const string& name);

    // Очищает список сэйверов
    void clearSavers(bool clearStd = true);

    // Возвращает snapshot пользовательских сэйверов
    SaverList savers() const;

    // Возвращает максимальный уровень логирования для сэйверов зарегистированных
    // на данный момент в логгере.
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
    //MessageList _messagesBuff;
    mutable atomic_flag  _messagesLock = ATOMIC_FLAG_INIT;
    //mutable std::mutex _messagesLock;

    SaverLPtr _saverOut;  // Сэйвер для STDOUT
    SaverLPtr _saverErr;  // Сэйвер для STDERR
    SaverList _savers;    // Список CUSTOM-сэйверов
    mutable atomic_flag  _saversLock = ATOMIC_FLAG_INIT;
    //mutable std::mutex  _saversLock;

    volatile Level _level = {NONE};

    int _flushTime = {300};
    int _flushSize = {1000};
    volatile bool _forceFlush = {false};
    volatile bool _on = {true};

    friend struct Line;
    template<typename T, int> friend T& ::safe_singleton();
};
Logger& logger();


//---------------------------- Line operators --------------------------------

inline bool Line::toLogger() const
{
    return (impl) ? (impl->level <= impl->logger->level()) : false;
}

template<typename T>
inline Line& operator<< (Line& line, const T& t)
{
    if (line.toLogger())
        line.impl->buff << t;
    return line;
}

template<typename T>
inline Line operator<< (Line&& line, const T& t)
{
    if (line.toLogger())
        line.impl->buff << t;
    return std::move(line);
}

inline Line& operator<< (Line& line, bool b)
{
    if (line.toLogger())
        line.impl->buff << (b ? "true" : "false");
    return line;
}

inline Line operator<< (Line&& line, bool b)
{
    if (line.toLogger())
        line.impl->buff << (b ? "true" : "false");
    return std::move(line);
}

} // namespace alog

//inline alog::Logger& logger() {return alog::logger();}


//#if defined(_MSC_VER)
//#define  LOGGER_FUNC_NAME  __FUNCTION__
//#else
//#define  LOGGER_FUNC_NAME  __PRETTY_FUNCTION__
//#endif
#define LOGGER_FUNC_NAME  __func__

#define log_error   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_warn    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_info    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_verbose alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_debug   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_debug2  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__)
