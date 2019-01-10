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
*****************************************************************************/

#include "logger.h"
#include "break_point.h"
#include "spin_locker.h"
#include "steady_timer.h"
#include "utils.h"

#include <ctime>
#include <stdexcept>
#include <algorithm>
#include <string.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#include <windows.h>
#endif

namespace alog {

using namespace std;

// Функция записывает сообщения об ошибке произошедшей в самом логгере.
// Информация сохраняется в файл /tmp/alogger.log для Linux/Unix,
// и в файл %TEMP%\\alogger.log для Windows.
void loggerPanic(const char* saverName, const char* error)
{
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
    char filePath[MAX_PATH + 12 /*alogger.log*/] = {0};
    GetTempPathA(MAX_PATH - 1, filePath);
    strcat(filePath, "alogger.log");
    if (FILE* f = fopen(filePath, "a"))
#else
    if (FILE* f = fopen("/tmp/alogger.log", "a"))
#endif
    {
        fputs("Saver name: ", f);
        fputs(saverName, f);
        fputs("; ", f);
        fputs("Error: ", f);
        fputs(error, f);
        fputs("\n", f);
        fclose(f);
    }
}

Level levelFromString(const string& level)
{
    if (level == "none")
        return Level::None;
    else if (level == "error")
        return Level::Error;
    else if (level == "warning")
        return Level::Warning;
    else if (level == "info")
        return Level::Info;
    else if (level == "verbose")
        return Level::Verbose;
    else if (level == "debug")
        return Level::Debug;
    else if (level == "debug2")
        return Level::Debug2;

    return Level::Info;
}

static const char* levelToStringImpl(Level level)
{
    switch (level)
    {
        // Примечание: пробелы в конце строк удалять нельзя, так как
        // это скажется на производительности функции prefixFormatter2().
        case Level::None:    return "NONE    ";
        case Level::Error:   return "ERROR   ";
        case Level::Warning: return "WARNING ";
        case Level::Info:    return "INFO    ";
        case Level::Verbose: return "VERBOSE ";
        case Level::Debug:   return "DEBUG   ";
        case Level::Debug2:  return "DEBUG2  ";
        default:             return "UNKNOWN ";
    }
}

string levelToString(Level level)
{
    string s = levelToStringImpl(level);
    return utl::rtrim(s);
}

// Формирует префикс строки лога. В префикс входит время и дата записи, уровень
// логирования,  номер потока, наименование функции из которой выполнен вызов.
void prefixFormatter(Message& message, time_t& lastTime, char buff[sizeof(Message::prefix)])
{
    if (lastTime != message.timeVal.tv_sec)
    {
        std::tm tm;
        memset(&tm, 0, sizeof(tm));
        memset(buff, 0, sizeof(Message::prefix));

        lastTime = message.timeVal.tv_sec;
        localtime_r(&lastTime, &tm);

        snprintf(buff, sizeof(Message::prefix) - 1,
                 "%02d.%02d.%04d %02d:%02d:%02d",
                 tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    memcpy(message.prefix, buff, sizeof(Message::prefix));
}

void prefixFormatter2(Message& message)
{
    char buff[sizeof(Message::prefix2)] = {0};
    long tv_usec = long(message.timeVal.tv_usec);

    snprintf(buff, sizeof(buff) - 1, ".%06ld", tv_usec);
    memcpy(message.prefix2, buff, sizeof(buff));
}

void prefixFormatter3(Message& message)
{
    char buff[sizeof(Message::prefix3)] = {0};

    const char* level = levelToStringImpl(message.level);
    long tid = long(message.threadId);

    char module[50] = {0};
    if (!message.module.empty())
        snprintf(module, sizeof(module) - 1, "%s", message.module.c_str());

    //if (!message.file.empty())
    //    snprintf(buff, sizeof(buff) - 1, " %sLWP%ld [%s:%d:%s%s] ",
    //             level, tid, message.file.c_str(), message.line, message.func.c_str(), module);
    //else
    //    snprintf(buff, sizeof(buff) - 1, " %sLWP%ld %s  ", level, tid, module);
    if (!message.file.empty())
    {
        if (!message.module.empty())
            snprintf(buff, sizeof(buff) - 1, " %sLWP%ld [%s:%d %s] ",
                     level, tid, message.file.c_str(), message.line, module);
        else
            snprintf(buff, sizeof(buff) - 1, " %sLWP%ld [%s:%d] ",
                     level, tid, message.file.c_str(), message.line);
    }
    else if (!message.module.empty())
        snprintf(buff, sizeof(buff) - 1, " %sLWP%ld [%s] ", level, tid, module);
    else
        snprintf(buff, sizeof(buff) - 1, " %sLWP%ld ", level, tid);

    memcpy(message.prefix3, buff, sizeof(buff));
}

//-------------------------------- Filter ------------------------------------

void Filter::setName(const string& name)
{
    if (locked()) return;
    _name = name;
}

void Filter::setMode(Mode val)
{
    if (locked()) return;
    _mode = val;
}

void Filter::setFilteringErrors(bool val)
{
    if (locked()) return;
    _filteringErrors = val;
}

bool Filter::followThreadContext() const
{
    return _followThreadContext;
}

void Filter::setFollowThreadContext(bool val)
{
    if (locked()) return;
    _followThreadContext = val;
}

Filter::Check Filter::check(const Message& m) const
{
    if (!_locked)
        return Check::NoLock;

    if ((m.level == Error) && !_filteringErrors)
        return Check::MessError;

    if (checkImpl(m))
    {
        if (_followThreadContext)
        {
            if (_mode == Mode::Include)
                _threadContextIds[m.threadId] = m.timeVal;

            if (_mode == Mode::Exclude)
            {
                if (_threadContextIds.find(m.threadId) != _threadContextIds.end())
                    return Check::Fail;
            }
        }
        return Check::Success;
    }

    if (_followThreadContext)
    {
        if (_mode == Mode::Exclude)
            _threadContextIds[m.threadId] = m.timeVal;

        if (_mode == Mode::Include)
        {
            if (_threadContextIds.find(m.threadId) != _threadContextIds.end())
                return Check::Success;
        }
    }
    return Check::Fail;
}

void Filter::removeIdsTimeoutThreads()
{
    if (_threadContextIds.empty())
        return;

    timeval curTime;
    gettimeofday(&curTime, NULL);

    vector<pid_t> tids;
    for (const auto& tci : _threadContextIds)
    {
        const timeval& timeVal = tci.second;
        // Таймаут в 3 сек.
        if (curTime.tv_sec > (timeVal.tv_sec + 3)
            || ((curTime.tv_sec == (timeVal.tv_sec + 3))
                && (curTime.tv_usec > timeVal.tv_usec)))
        {
            tids.push_back(tci.first);
        }
    }
    for (pid_t tid : tids)
        _threadContextIds.erase(tid);
}

//------------------------------ FilterModule --------------------------------

void FilterModule::addModule(const string& name)
{
    if (locked()) return;
    _modules.insert(name);
}

void FilterModule::setFilteringNoNameModules(bool val)
{
    if (locked()) return;
    _filteringNoNameModules = val;
}

bool FilterModule::checkImpl(const Message& m) const
{
    if (m.module.empty() && !_filteringNoNameModules)
        return true;

    bool res = (_modules.find(m.module) != _modules.end());
    return (mode() == Mode::Exclude) ? !res : res;
}

//----------------------------- FilterLevel ----------------------------------

void FilterLevel::setLevel(Level val)
{
    if (locked()) return;
    _level = val;
}

bool FilterLevel::checkImpl(const Message& m) const
{
    if (m.module.empty() && !filteringNoNameModules())
        return true;

    if (_level == None)
        return true;

    if (mode() == Mode::Include)
    {
        if (modules().find(m.module) == modules().end())
            return true;

        return (m.level <= _level);
    }

    // Для mode() == Mode::Exclude
    if (modules().find(m.module) != modules().end())
        return true;

    return (m.level <= _level);
}

//------------------------------ FilterFile ----------------------------------

void FilterFile::addFile(const string& name)
{
    if (locked()) return;
    _files.insert(name);
}

bool FilterFile::checkImpl(const Message& m) const
{
    bool res  = (_files.find(m.file) != _files.end());
    return (mode() == Mode::Exclude) ? !res : res;
}

//------------------------------- FilterFunc ---------------------------------

void FilterFunc::addFunc(const string& name)
{
    if (locked()) return;
    _funcs.insert(name);
}

bool FilterFunc::checkImpl(const Message& m) const
{
    bool res  = (_funcs.find(m.func) != _funcs.end());
    return (mode() == Mode::Exclude) ? !res : res;
}

//------------------------------ FilterThread --------------------------------

bool FilterThread::followThreadContext() const
{
    return false;
}

void FilterThread::addThread(long id)
{
    if (locked()) return;
    _threads.insert(pid_t(id));
}

bool FilterThread::checkImpl(const Message& m) const
{
    bool res  = (_threads.find(m.threadId) != _threads.end());
    return (mode() == Mode::Exclude) ? !res : res;
}

//--------------------------------- Saver ------------------------------------

Saver::Saver(const string& name, Level level)
    : _name(name),
      _level(level)
{}

void Saver::setActive(bool val)
{
    _active = val;
    if (Logger* logger = _logger.load())
        logger->redefineLevel();
}

void Saver::flush(const MessageList& messages)
{
    if (!_active)
        return;

    if (_level == Level::None)
        return;

    flushImpl(messages);
}

FilterList Saver::filters() const
{
    FilterList filters;
    SpinLocker locker(_filtersLock); (void) locker;
    for (int i = 0; i < _filters.count(); ++i)
    {
        Filter* f = _filters.item(i);
        f->add_ref();
        filters.add(f);
    }
    return std::move(filters);
}

void Saver::addFilter(FilterPtr filter)
{
    SpinLocker locker(_filtersLock); (void) locker;
    lst::FindResult fr = _filters.findRef(filter->name(),
                                          lst::FindExtParams(lst::BruteForce::Yes));
    if (fr.success())
        _filters.remove(fr.index());

    filter->lock();
    filter->add_ref();
    _filters.add(filter.get());
}

void Saver::removeFilter(const string& name)
{
    SpinLocker locker(_filtersLock); (void) locker;
    lst::FindResult fr = _filters.findRef(name, lst::FindExtParams(lst::BruteForce::Yes));
    if (fr.success())
        _filters.remove(fr.index());
}

void Saver::clearFilters()
{
    SpinLocker locker(_filtersLock); (void) locker;
    _filters.clear();
}

bool Saver::skipMessage(const Message& m, const FilterList& filters)
{
    if (filters.empty())
        return false;

    for (Filter* filter : filters)
    {
        Filter::Check res = filter->check(m);
        if (res == Filter::Check::MessError)
        {
            // Прерываем фильтрацию на первом фильтре, который не фильтрует
            // сообщения об ошибках.
            return false;
        }
        else if (res == Filter::Check::Fail)
            return true;
    }

    // Если попали в эту точку - значит результат проверки последнего фильтра
    // равен Filter::Check::Success или Filter::Check::NoLock. В обоих случаях
    // сообщение не должно исключаться из вывода в лог-файл.
    return false;
}

void Saver::removeIdsTimeoutThreads()
{
    FilterList filters = this->filters();
    for (Filter* filter : filters)
        filter->removeIdsTimeoutThreads();
}

//------------------------------ SaverStdOut ---------------------------------

SaverStdOut::SaverStdOut(const char* name, Level level, bool shortMessages)
    : Saver(name, level)
{
    _out = &std::cout;
    _shortMessages = shortMessages;
}

void SaverStdOut::flushImpl(const MessageList& messages)
{
    if (messages.size() == 0)
        return;

    removeIdsTimeoutThreads();

    vector<char> lineBuff;
    if (maxLineSize() > 0)
    {
        lineBuff.resize(maxLineSize() + 1);
        lineBuff[maxLineSize()] = '\0';
    }

    unsigned flushCount = 0;
    FilterList filters = this->filters();

    for (Message* m : messages)
    {
        if (m->level > level())
            continue;

        if (skipMessage(*m, filters))
            continue;

        if (!_shortMessages)
        {
            (*_out) << m->prefix;
            if (level() == Level::Debug2)
                (*_out) << m->prefix2;
            (*_out) << m->prefix3;
        }
        if ((maxLineSize() > 0) && (maxLineSize() < int(m->str.size())))
        {
            strncpy(&lineBuff[0], m->str.c_str(), maxLineSize());
            (*_out) << (char*) &lineBuff[0];
        }
        else
            (*_out) << m->str;

        (*_out) << "\n";

        if (++flushCount % 50 == 0)
            _out->flush();
    }
    _out->flush();
}

//------------------------------ SaverStdErr ---------------------------------

SaverStdErr::SaverStdErr(const char* name, Level level, bool shortMessages)
    : SaverStdOut(name, level, shortMessages)
{
    _out = &std::cerr;
}

//------------------------------ SaverFile -----------------------------------

SaverFile::SaverFile(const string& name,
                     const string& filePath,
                     Level level,
                     bool isContinue)
    : Saver(name, level),
      _filePath(filePath),
      _isContinue(isContinue)
{
    if (!_isContinue)
    {
        // Очищаем существующий файл
        if (FILE* f = fopen(_filePath.c_str(), "w"))
            fclose(f);
        else
            throw std::logic_error("Could not open file: " + _filePath);
    }
}

void SaverFile::flushImpl(const MessageList& messages)
{
    if (messages.size() == 0)
        return;

    if (FILE* f = fopen(_filePath.c_str(), "a"))
    {
        removeIdsTimeoutThreads();

        vector<char> lineBuff;
        if (maxLineSize() > 0)
        {
            lineBuff.resize(maxLineSize() + 1);
            lineBuff[maxLineSize()] = '\0';
        }

        unsigned flushCount = 0;
        FilterList filters = this->filters();

        for (Message* m : messages)
        {
            if (m->level > level())
                continue;

            if (skipMessage(*m, filters))
                continue;

            fputs(m->prefix, f);
            if (level() == Level::Debug2)
                fputs(m->prefix2, f);
            fputs(m->prefix3, f);

            if ((maxLineSize() > 0) && (maxLineSize() < int(m->str.size())))
            {
                strncpy(&lineBuff[0], m->str.c_str(), maxLineSize());
                fputs(&lineBuff[0], f);
            }
            else
                fputs(m->str.c_str(), f);

            fputs("\n", f);

            if (++flushCount % 500 == 0)
                fflush(f);
        }
        fflush(f);
        fclose(f);
    }
    else
    {
        throw std::logic_error("Could not open file: " + _filePath);
    }
}

//--------------------------------- Line -------------------------------------

Line::Line(Logger*     logger,
           Level       level,
           const char* file,
           const char* func,
           int         line,
           const char* module)
    : impl(new Impl)
{
    impl->logger = logger;
    impl->level  = level;
    impl->file   = file;
    impl->func   = func;
    impl->line   = line;
    impl->module = module;
}

Line::~Line()
{
    if (impl.empty())
        return;

    if (impl->logger->threadStop() || !impl->logger->_on)
        return;

    if (impl->level > impl->logger->level())
        return;

    try
    {
        MessagePtr message {new Message};
        message->level = impl->level;
        message->str = impl->buff.str();
        gettimeofday(&message->timeVal, NULL);
        message->threadId = trd::gettid();

        if (impl->file)
        {
            const char* f = strrchr(impl->file, '/') + 1;
            message->file = (f) ? f : impl->file;
        }
        if (impl->func)
            message->func = impl->func;

        message->line = impl->line;
        if (impl->module)
            message->module = impl->module;

        impl->logger->addMessage(std::move(message));

        if (impl->level == Error)
            impl->logger->flush();
    }
    catch (...)
    {}
}

//------------------------------ LevelProxy ----------------------------------

LevelProxy::LevelProxy(Logger*     logger,
                       Level       level,
                       const char* file,
                       const char* func,
                       int         line,
                       const char* module)
    : logger(logger),
      level(level),
      file(file),
      func(func),
      line(line),
      module(module)
{}

//--------------------------------- Logger -----------------------------------

Logger::Logger()
{}

Logger::~Logger()
{
    flush();
    waitingFlush();

    stop();
}

void Logger::addMessage(MessagePtr&& m)
{
    SpinLocker locker(_messagesLock); (void) locker;
    _messages.add(m.release());
}

void Logger::run()
{
    steady_timer flushTimer;
    MessageList messagesBuff;

    // Вспомогательный флаг, нужен чтобы дать возможность перед прерываением
    // потока сделать лишний цикл while (true) и сбросить все буферы в сэйверы.
    // Примечание: threadStop() для этой цели использовать нельзя.
    bool loopBreak = false;

    auto saverFlush = [] (const MessageList& messages, Saver* saver)
    {
        if (messages.empty())
            return;
        try
        {
            saver->flush(messages);
        }
        catch (std::exception& e)
        {
            loggerPanic(saver->name().c_str(), e.what());
        }
        catch (...)
        {
            loggerPanic(saver->name().c_str(), "unknown error");
        }
    };

    while (true)
    {
        int messagesCount;
        { //Блок для SpinLocker
            SpinLocker locker(_messagesLock); (void) locker;
            messagesCount = _messages.count();
        }

        if (!threadStop() && (messagesCount == 0))
        {
            static chrono::milliseconds sleepThread {20};
            this_thread::sleep_for(sleepThread);
        }

        MessageList messages;
        { //Блок для SpinLocker
            SpinLocker locker(_messagesLock); (void) locker;
            messages.swap(_messages);
        }
        if (!threadStop()
            && messages.count() == 0
            && messagesBuff.count() == 0)
        {
            _forceFlush = false;
            continue;
        }

        if (messages.count())
        {
            auto prefixFormatterL = [this] (MessageList& messages, int min, int max)
            {
                time_t lastTime = 0;
                char prefixBuff[sizeof(Message::prefix)] = {0};
                Level level = this->level(); // volatile оптимизация
                for (int i = min; i < max; ++i)
                {
                    prefixFormatter(messages[i], lastTime, prefixBuff);
                    if (level == Level::Debug2)
                        prefixFormatter2(messages[i]);
                    prefixFormatter3(messages[i]);
                }
            };

            int threadsCount = 0;
            if (messages.count() > 50000)  ++threadsCount;
            if (messages.count() > 100000) ++threadsCount;
            if (messages.count() > 150000) ++threadsCount;

            int step = messages.count();
            int threadIndex = 0;
            vector<thread> threads;
            if (threadsCount)
            {
                step = messages.count() / (threadsCount + 1);
                for (; threadIndex < threadsCount; ++threadIndex)
                    threads.push_back(thread(prefixFormatterL,
                                             std::ref(messages),
                                             threadIndex * step,
                                             (threadIndex + 1) * step));
            }
            prefixFormatterL(messages, threadIndex * step, messages.count());

            for (size_t i = 0; i < threads.size(); ++i)
                threads[i].join();

            SaverPtr saverOut;
            SaverPtr saverErr;

            { //Блок для SpinLocker
                SpinLocker locker(_saversLock); (void) locker;
                saverOut = _saverOut;
                saverErr = _saverErr;
            }
            if (saverOut)
                saverFlush(messages, saverOut.get());
            if (saverErr)
                saverFlush(messages, saverErr.get());

            for (int i = 0; i < messages.count(); ++i)
                messagesBuff.add(messages.release(i, lst::CompressList::No));
            messages.clear();

        } //if (messages.count())

        if (loopBreak
            || _forceFlush
            || flushTimer.elapsed() > _flushTime
            || messagesBuff.count() > _flushSize)
        {
            flushTimer.reset();
            if (messagesBuff.count())
            {
                SaverList savers = this->savers();
                for (Saver* saver : savers)
                    saverFlush(messagesBuff, saver);
            }
            _forceFlush = false;
            messagesBuff.clear();
        }

        if (loopBreak)
            break;
        if (threadStop())
            loopBreak = true;
    } //while (true)
}

void Logger::flush()
{
    _forceFlush = true;
}

void Logger::waitingFlush()
{
    while (_forceFlush && !threadStop())
    {
        static chrono::milliseconds sleepThread {20};
        this_thread::sleep_for(sleepThread);
    }
}

LevelProxy Logger::error_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, Error, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::warn_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, Warning, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::info_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, Info, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::verbose_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, Verbose, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::debug_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, Debug, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::debug2_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, Debug2, file, func, line, module);
    return std::move(lp);
}

void Logger::addSaverStdOut(Level level, bool shortMessages)
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        _saverOut = SaverPtr(new SaverStdOut("stdout", level, shortMessages));
        _saverOut->setLogger(this);
    }
    redefineLevel();
}

void Logger::addSaverStdErr(Level level, bool shortMessages)
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        _saverErr = SaverPtr(new SaverStdErr("stderr", level, shortMessages));
        _saverErr->setLogger(this);
    }
    redefineLevel();
}

void Logger::removeSaverStdOut()
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        _saverOut.reset();
    }
    redefineLevel();
}

void Logger::removeSaverStdErr()
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        _saverErr.reset();
    }
    redefineLevel();
}

void Logger::addSaver(SaverPtr saver)
{
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        lst::FindResult fr = _savers.findRef(saver->name(), {lst::BruteForce::Yes});
        if (fr.success())
            _savers.remove(fr.index());

        saver->add_ref();
        saver->setLogger(this);
        _savers.add(saver.get());
    }
    redefineLevel();
}

void Logger::removeSaver(const string& name)
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        lst::FindResult fr = _savers.findRef(name, {lst::BruteForce::Yes});
        if (fr.success())
        {
            _savers.item(fr.index())->setLogger(0);
            _savers.remove(fr.index());
        }
    }
    redefineLevel();
}

SaverPtr Logger::findSaver(const string& name)
{
    SaverList savers = this->savers();
    Saver* saver = savers.findItem(&name, {lst::BruteForce::Yes});
    return SaverPtr(saver);
}

void Logger::clearSavers(bool clearStd)
{
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        if (clearStd)
        {
            _saverOut.reset();
            _saverErr.reset();
        }
        for (Saver* saver : _savers)
            saver->setLogger(0);
        _savers.clear();
    }
    redefineLevel();
}

SaverList Logger::savers() const
{
    SaverList savers;
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        for (Saver* s : _savers)
        {
            s->add_ref();
            savers.add(s);
        }
    }
    return std::move(savers);
}

void Logger::redefineLevel()
{
    Level level = None;
    SaverPtr saverOut;
    SaverPtr saverErr;

    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        saverOut = _saverOut;
        saverErr = _saverErr;
    }
    if (saverOut && saverOut->active() && (saverOut->level() > level))
        level = saverOut->level();
    if (saverErr && saverErr->active() && (saverErr->level() > level))
        level = saverErr->level();

    SaverList savers = this->savers();
    for (Saver* saver : savers)
        if (saver->active() && (saver->level() > level))
            level = saver->level();

    _level = level;
}

Logger& logger()
{
    return ::safe_singleton<Logger>();
}

//---------------------------- Line operators --------------------------------

Line& operator<< (Line& line, bool b)
{
    if (line.toLogger())
        line.impl->buff << (b ? "true" : "false");
    return line;
}

Line operator<< (Line&& line, bool b)
{
    if (line.toLogger())
        line.impl->buff << (b ? "true" : "false");
    return std::move(line);
}

Line& operator<< (Line& line, const char* c)
{
    if (line.toLogger())
        line.impl->buff << c;
    return line;
}

Line operator<< (Line&& line, const char* c)
{
    if (line.toLogger())
        line.impl->buff << c;
    return std::move(line);
}

Line& operator<< (Line& line, const timeval& tv)
{
    if (line.toLogger())
    {
        char buff[10] = {0};
        long tv_usec = long(tv.tv_usec);
        snprintf(buff, sizeof(buff) - 1, ".%06ld", tv_usec);
        buff[5] = '\0';
        line.impl->buff << tv.tv_sec << buff;
    }
    return line;
}

Line operator<< (Line&& line, const timeval& tv)
{
    if (line.toLogger())
    {
        char buff[10] = {0};
        long tv_usec = long(tv.tv_usec);
        snprintf(buff, sizeof(buff) - 1, ".%06ld", tv_usec);
        buff[5] = '\0';
        line.impl->buff << tv.tv_sec << buff;
    }
    return std::move(line);
}

} // namespace alog
