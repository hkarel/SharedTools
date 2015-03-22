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
// обозначения уровня логгирования в enum Level{};
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
    // при записи сообщения сразу в несколько сейверов. Причина: большое потреб-
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
  Базовый класс механизма сохранения
*/
class Saver : public clife_base
{
public:
    //enum Type {STDOUT, STDERROR, CUSTOM};

    Saver(const string& name, Level level = ERROR);
    virtual ~Saver() {}

    //Type type() const {return _type;}
    const string& name() const {return _name;}

    // Уровень логгирования
    Level level() const {return _level;}
    void setLevel(Level val) {_level = val;}

    // Выполняет запись буфера сообщений
    void flush(const MessageList&);

    struct Allocator
    {
        void destroy(Saver* x) {if (x) x->release();}
    };
    struct Find
    {
        int operator() (const string* name, const Saver* item2, void* = 0) const
            {return name->compare(item2->name());}

        //int operator() (const Type* type, const Saver* item2, void* = 0) const
        //    {return LIST_COMPARE_ITEM(*type, item2->type());}
    };

protected:
    virtual void flushImpl(const MessageList&) = 0;

private:
    Level  _level = ERROR;
    //Type   _type  = CUSTOM;
    string _name;

    friend class SaverStdOut;
    friend class SaverStdErr;
};
typedef lst::List<Saver, Saver::Find, Saver::Allocator> SaverList;
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
    // вывод будет пересоздан с новым уровнем логгирования.
    void addSaverStdOut(Level level = ERROR);

    // Добавляет вывод логов в stderr. Если вывод уже был добавлен ранее, то
    // вывод будет пересоздан с новым уровнем логгирования.
    void addSaverStdErr(Level level = ERROR);

    // Запрещает вывод логов в stdout
    void removeSaverStdOut();

    // Запрещает вывод логов в stderr
    void removeSaverStdErr();

    // Включает вывод информации в логи (по умолчанию включено)
    void on() noexcept {_on = true;}

    // Позволяет верменно отключить вывод данных в логи
    void off() noexcept {_on = false;}

    // Определяет интерывал записи сообщений для CUSTOM-сэйверов. Измеряется
    // в миллисекундах, значение по умолчанию 300 ms.
    int  flushTime() const {return _flushTime;}
    void setFlushTime(int val) {_flushTime = val;}

    // Определяет размер буфера после которого будет выполена запись сообщений
    // для CUSTOM-сэйверов. Значение по умолчанию 1000.
    int  flushSize() const {return _flushSize;}
    void setFlushSize(int val) {_flushSize = val;}

    // Добавляет сэйвер в список сэйверов. Если сэйвер с указанным именем уже
    // существует, то он будет заменен новым.
    void addSaver(const SaverLPtr&);
    void removeSaver(const string& name);

    // Возвращает snapshot пользовательских сэйверов
    SaverList savers() const;

    // Возвращает максимальный уровень логгирования для сейверов зарегистированных
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

template<typename T>
Line& operator<< (Line& line, const T& t)
{
    if (line.impl->level <= line.impl->logger->level())
        line.impl->buff << t;
    return line;
}

template<typename T>
Line operator<< (Line&& line, const T& t)
{
    if (line.impl->level <= line.impl->logger->level())
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
