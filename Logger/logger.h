/****************************************************************************
  Огиринальная идея логгера: Евдокимов Михаил
  Переработка: Карелин Павел

  Реализация высоконагруженного асинхронного логгера.

****************************************************************************/

#pragma once

#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <atomic>
#include <memory>
#include <vector>
#include <set>
#include <chrono>
#include <thread>
#include <mutex>

#include "Thread/thread_base.h"
#include "_list.h"
#include "clife_base.h"
#include "clife_ptr.h"
#include "simple_ptr.h"
#include "safe_singleton.h"


namespace lblog
{
using namespace std;

class Logger;


// Уровни log-сообщений
enum Level {NONE, ERROR, WARNING, INFO, VERBOSE, DEBUG, DEBUG2};

// Вспомогательная функция, используется для преобразования строкового
// обозначения уровня логирования в enum Level{};
Level levelFromString(const string& level);


/**
  Базовое сообщение
*/
struct Message //: public clife_base
{
    Level  level;
    string str;
    string module;

    // Буферы prefix и prefix2 хранят результаты функций prefixFormatter2().
    // Основное назначение минимизировать количество вызовов prefixFormatter2()
    // при записи сообщения сразу в несколько сэйверов. Причина: большое потреб-
    // ление системных ресурсов при многократном вызове prefixFormatter2().
    // Важно: prefix-буферы специально сделаны выделяемыми на стеке,
    // т.к. память для них должна выделяться в момент создания объекта, а не
    // в момент заполнения префиксов.
    char prefix[30] = {0};
    char prefix2[300] = {0};

    string file;
    string func;
    int    line;

    struct timeval timeVal;
    pthread_t threadId;

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
    int operator() (const string* name, const T* item2, void* = 0) const
        {return name->compare(item2->name());}

    //int operator() (const Type* type, const Saver* item2, void* = 0) const
    //    {return LIST_COMPARE_ITEM(*type, item2->type());}
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
    enum Mode {Include, Exclude};

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

    // Возвращает значение большее нуля в случае если сообщение соответствует
    // критериям фильтрации, если сообщение не соответствует критериям фильтрации
    // будет возвращен нуль. В случае если фильтр не заперт - будет возвращено
    // значение меньшее нуля.
    int check(const Message&) const;

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

private:
    string _name;
    Mode   _mode = {Include};
    bool   _locked = {false};
    bool   _filteringErrors = {false};
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

private:
    bool checkImpl(const Message&) const override;
    set<string> _modules;
};
typedef clife_ptr<FilterModule> FilterModuleLPtr;


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
    Level _level = {NONE};
};
typedef clife_ptr<FilterLevel> FilterLevelPtr;


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

    // Уровень логирования
    Level level() const {return _level;}
    void setLevel(Level val) {_level = val;}

    // Выполняет запись буфера сообщений
    void flush(const MessageList&);

    // Устанавливает ограничение на максимальную длину строки сообщения.
    // Длина строки не ограничивается если значение меньше либо равно 0.
    // Значение по умолчанию 5000.
    // TODO: Для utf8 кодировки учесть, чтобы обрезка не происходила посередине
    //       символов.
    int  maxLineSize() const {return _maxLineSize;}
    void setMaxLineSize(int val) {_maxLineSize = val;}

    // Возвращает список фильтров
    FilterList filters() const;

    // Добавляет фильтр в список фильтров. Если фильтр с указанным именем уже
    // существует, то он будет заменен новым.
    void addFilter(FilterLPtr);

    // Удаляет фильтр
    void removeFilter(const string& name);

    // Очищает список фильтров
    void clearFilters();

    // Проверяет принадлежит ли сообщение фильтрам
    static bool skipMessage(const Message& m, const FilterList& filters);

protected:
    virtual void flushImpl(const MessageList&) = 0;

private:
    string _name;
    Level  _level = {ERROR};

    int _maxLineSize = {5000};

    FilterList _filters;
    mutable atomic_flag  _filtersLock = ATOMIC_FLAG_INIT;

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
    SaverStdOut(Level level = ERROR);
    void flushImpl(const MessageList&) override;

protected:
    ostream* _out;
};


/**
  Вывод в stderr
*/
class SaverStdErr : public SaverStdOut
{
public:
    SaverStdErr(Level level = ERROR);
};


/**
  Вывод в файл
*/
class SaverFile : public Saver
{
public:
    SaverFile(const string& name, const string& fileName, Level level = ERROR);
    void flushImpl(const MessageList&) override;

private:
    string _fileName;
};


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

    Line() = delete;
    Line(Line&&) = default;
    Line(const Line&) = delete;
    Line& operator= (Line&&) = delete;
    Line& operator= (const Line&) = delete;

    // Сервисная функция, выполняет проверку уровня логирования и определяет
    // нужно ли добавлять сообщение в логер.
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
    typedef chrono::high_resolution_clock exact_clock;

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
    void flush(int pauseDuration = 0);

    // Добавляет вывод логов в stdout. Если вывод уже был добавлен ранее, то
    // вывод будет пересоздан с новым уровнем логирования.
    void addSaverStdOut(Level level = ERROR);

    // Добавляет вывод логов в stderr. Если вывод уже был добавлен ранее, то
    // вывод будет пересоздан с новым уровнем логирования.
    void addSaverStdErr(Level level = ERROR);

    // Запрещает вывод логов в stdout
    void removeSaverStdOut();

    // Запрещает вывод логов в stderr
    void removeSaverStdErr();

    // Включает вывод информации в логи (по умолчанию включено)
    void on() noexcept {_on = true;}

    // Позволяет верменно отключить вывод данных в логи
    void off() noexcept {_on = false;}

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

    // Очищает список сэйверов
    void clearSavers();

    // Возвращает snapshot пользовательских сэйверов
    SaverList savers() const;

    // Возвращает максимальный уровень логирования для сэйверов зарегистированных
    // на данный момент в логгере.
    Level level() const {return _level;}

private:
    Logger();
    Logger(Logger&&) = delete;
    Logger(const Logger&) = delete;
    Logger& operator= (Logger&&) = delete;
    Logger& operator= (const Logger&) = delete;

    void addMessage(MessagePtr&&);
    void run() override;

    void redefineLevel();

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

inline Logger& logger() {return ::safe_singleton<Logger>();}


//---------------------------- Line operators -------------------------------

inline bool Line::toLogger() const
{
    return (impl) ? (impl->level <= impl->logger->level()) : false;
}

template<typename T>
Line& operator<< (Line& line, const T& t)
{
    //if (line.impl->level <= line.impl->logger->level())
    if (line.toLogger())
        line.impl->buff << t;
    return line;
}

template<typename T>
Line operator<< (Line&& line, const T& t)
{
    //if (line.impl->level <= line.impl->logger->level())
    if (line.toLogger())
        line.impl->buff << t;
    return std::move(line);
}



} // namespace lblog

inline lblog::Logger& logger() {return lblog::logger();}


//#if defined(_MSC_VER)
//#define  LOGGER_FUNC_NAME  __FUNCTION__
//#else
//#define  LOGGER_FUNC_NAME  __PRETTY_FUNCTION__
//#endif
#define LOGGER_FUNC_NAME  __func__

#define log_error   logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_warn    logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_info    logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_verbose logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_debug   logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__)
#define log_debug2  logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__)
