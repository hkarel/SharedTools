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

#include "logger/logger.h"
#include "break_point.h"
#include "spin_locker.h"
#include "steady_timer.h"

#include <string.h>
#include <algorithm>
#include <ctime>
#include <stdexcept>
#include <vector>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#include <windows.h>
#endif

namespace alog {

using namespace std;

// Функция записывает сообщения об ошибке произошедшей в самом логгере.
// Информация сохраняется в файл /tmp/alogger.log для Linux/Unix,
// и в файл %TEMP%\\alogger.log для Windows
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
    if      (level == "none"   ) return Level::None;
    else if (level == "error"  ) return Level::Error;
    else if (level == "warning") return Level::Warning;
    else if (level == "info"   ) return Level::Info;
    else if (level == "verbose") return Level::Verbose;
    else if (level == "debug"  ) return Level::Debug;
    else if (level == "debug2" ) return Level::Debug2;
    else                         return Level::Info;
}

static const char* levelToStringImpl(Level level)
{
    switch (level)
    {
        // Примечание: пробелы  в конце строк удалять  нельзя, так как
        // это скажется на производительности функции prefixFormatter2()
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

    // См. реализацию utl::rtrim(), минимизация зависимости от модуля utils
    auto begin = std::find_if_not(s.rbegin(), s.rend(),
                 [](unsigned char c) {return std::isspace(c);}).base();
    s.erase(begin, s.end());
    return s;
}

#if __cplusplus >= 201703L && !defined(LOGGER_USE_SNPRINTF)
// Функции  prefixFormatter{123}  формируют  префикс  строки  лога.  В префикс
// входит время и дата записи, уровень логирования, номер потока, наименование
// файла, номер строки, имя модуля
void prefixFormatter1(Message& message, time_t& lastTime, char buff[sizeof(Message::prefix1)])
{
    if (lastTime != message.timeSpec.tv_sec)
    {
        std::tm tm;
        lastTime = message.timeSpec.tv_sec;

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
        localtime_s(&tm, &lastTime);
#else
        localtime_r(&lastTime, &tm);
#endif
        char* begin = buff;
        char* end = begin + sizeof(Message::prefix1);
        to_chars_result res;

        // Формат: "%02d.%02d.%04d %02d:%02d:%02d"

        if (tm.tm_mday < 10) *begin++ = '0';
        res = to_chars(begin, end, tm.tm_mday);
        begin = res.ptr;
        *begin++ = '.';
        if ((tm.tm_mon + 1) < 10) *begin++ = '0';
        res = to_chars(begin, end, tm.tm_mon + 1);
        begin = res.ptr;
        *begin++ = '.';

        res = to_chars(begin, end, tm.tm_year + 1900);
        begin = res.ptr;
        *begin++ = ' ';

        if (tm.tm_hour < 10) *begin++ = '0';
        res = to_chars(begin, end, tm.tm_hour);
        begin = res.ptr;
        *begin++ = ':';

        if (tm.tm_min < 10) *begin++ = '0';
        res = to_chars(begin, end, tm.tm_min);
        begin = res.ptr;
        *begin++ = ':';

        if (tm.tm_sec < 10) *begin++ = '0';
        res = to_chars(begin, end, tm.tm_sec);
        *res.ptr = '\0';
    }
    memcpy(message.prefix1, buff, sizeof(Message::prefix1));
}

template<size_t rsize>
void usecToString(int usec, char* result)
{
    static_assert(rsize >= 8, "Size of result must be at least 8");

    char* begin = result;
    char* end = begin + rsize;

    *begin++ = '.';
    if (usec >= 100000)
    {}
    else if (usec >= 10000)
    {
        *begin++ = '0';
    }
    else if (usec >= 1000)
    {
        *begin++ = '0';
        *begin++ = '0';
    }
    else if (usec >= 100)
    {
        for (int i = 0; i < 3; ++i) *begin++ = '0';
    }
    else if (usec >= 10)
    {
        for (int i = 0; i < 4; ++i) *begin++ = '0';
    }
    else // tv_usec < 10
    {
        for (int i = 0; i < 5; ++i) *begin++ = '0';
    }

    to_chars_result res = to_chars(begin, end, usec);
    if (res.ec != std::errc())
    {
        strcpy(result, "INVALID");
        return;
    }

    // Микросекунды не должны превышать 6 знаков (плюс 1 символ на '.')
    *(result + 7) = '\0';
}

void prefixFormatter2(Message& message)
{
    char buff[sizeof(Message::prefix2)];
    usecToString<sizeof(buff)>(message.timeSpec.tv_nsec / 1000, buff);

    memcpy(message.prefix2, buff, sizeof(buff));
}

void prefixFormatter3(Message& message)
{
    char buff[sizeof(Message::prefix3)];
    char* begin = buff;
    char* end = begin + sizeof(buff);
    to_chars_result res;
    size_t size, file_sz, module_sz;

    *begin++ = ' ';

    const char* level = levelToStringImpl(message.level);
    size = 8; // Размер message.level
    memcpy(begin, level, size);
    begin += size;

    *begin++ = 'L';
    *begin++ = 'W';
    *begin++ = 'P';

    int tid = int(message.threadId);
    res = to_chars(begin, end, tid);
    begin = res.ptr;

    #define STUB_NORMAL  \
        {*begin++ = ']'; \
         *begin++ = ' '; \
         *begin   = '\0';}

    #define STUB_EDGE      \
        {*(end - 3) = ']'; \
         *(end - 2) = ' '; \
         *(end - 1) = '\0';}

    if (message.file)
    {
        // Общий формат: " %sLWP%ld [%s:%d %s] "

        *begin++ = ' ';
        *begin++ = '[';

        size = end - begin;
        file_sz = strlen(message.file);
        memcpy(begin, message.file, min(size, file_sz));

        if (file_sz >= size)
        {
            STUB_EDGE
            memcpy(message.prefix3, buff, sizeof(buff));
            return;
        }
        begin += file_sz;

        if ((end - begin) > 3)
        {
            *begin++ = ':';
        }
        else
        {
            STUB_EDGE
            memcpy(message.prefix3, buff, sizeof(buff));
            return;
        }

        res = to_chars(begin, end, message.line);
        if (res.ec != std::errc())
        {
            if ((end - begin) > 3)
                STUB_NORMAL
            else
                STUB_EDGE

            memcpy(message.prefix3, buff, sizeof(buff));
            return;
        }
        begin = res.ptr;

        if (message.module)
        {
            if ((end - begin) > 3)
            {
                *begin++ = ' ';
            }
            else
            {
                STUB_EDGE
                memcpy(message.prefix3, buff, sizeof(buff));
                return;
            }

            size = end - begin;
            module_sz = strlen(message.module);
            memcpy(begin, message.module, min(size, module_sz));

            if (module_sz >= size)
            {
                STUB_EDGE
                memcpy(message.prefix3, buff, sizeof(buff));
                return;
            }
            begin += module_sz;
        }
    }
    else if (message.module)
    {
        // Сокращенный формат: " %sLWP%ld [%s] "

        *begin++ = ' ';
        *begin++ = '[';

        size = end - begin;
        module_sz = strlen(message.module);
        memcpy(begin, message.module, min(size, module_sz));
        begin += module_sz;
    }

    if ((end - begin) > 3)
        STUB_NORMAL
    else
        STUB_EDGE

    #undef STUB_NORMAL
    #undef STUB_EDGE

    memcpy(message.prefix3, buff, sizeof(buff));
}
#else // __cplusplus >= 201703L

// Функции  prefixFormatter{123}  формируют  префикс  строки  лога.  В префикс
// входит время и дата записи, уровень логирования, номер потока, наименование
// файла, номер строки, имя модуля
void prefixFormatter1(Message& message, time_t& lastTime, char buff[sizeof(Message::prefix1)])
{
    if (lastTime != message.timeSpec.tv_sec)
    {
        std::tm tm;
        lastTime = message.timeSpec.tv_sec;

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
        localtime_s(&tm, &lastTime);
#else
        localtime_r(&lastTime, &tm);
#endif

#pragma GCC diagnostic push
#if __GNUC__ > 6
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
        snprintf(buff, sizeof(Message::prefix1),
                 "%02d.%02d.%04d %02d:%02d:%02d",
                 tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

#pragma GCC diagnostic pop
    }
    memcpy(message.prefix1, buff, sizeof(Message::prefix1));
}

void prefixFormatter2(Message& message)
{
    char buff[sizeof(Message::prefix2)]; // = {0};
    long tv_usec = long(message.timeSpec.tv_nsec / 1000);

    snprintf(buff, sizeof(buff), ".%06ld", tv_usec);
    memcpy(message.prefix2, buff, sizeof(buff));
}

void prefixFormatter3(Message& message)
{
    char buff[sizeof(Message::prefix3)]; // = {0};
    buff[sizeof(buff) - 1] = '\0';

    const char* level = levelToStringImpl(message.level);
    long tid = long(message.threadId);

    if (message.file)
    {
        if (message.module)
            snprintf(buff, sizeof(buff) - 1, " %sLWP%ld [%s:%d %s] ",
                     level, tid, message.file, message.line, message.module);
        else
            snprintf(buff, sizeof(buff) - 1, " %sLWP%ld [%s:%d] ",
                     level, tid, message.file, message.line);
    }
    else if (message.module)
        snprintf(buff, sizeof(buff) - 1, " %sLWP%ld [%s] ", level, tid, message.module);
    else
        snprintf(buff, sizeof(buff) - 1, " %sLWP%ld ", level, tid);

    memcpy(message.prefix3, buff, sizeof(buff));
}
#endif // __cplusplus >= 201703L

//-------------------------------- Something ---------------------------------

bool Something::canModifyMessage() const
{
    return false;
}

string Something::modifyMessage(const string&) const
{
    return string();
}

//---------------------------------- Filter ----------------------------------

void Filter::setName(const string& name)
{
    if (locked())
        return;

    _name = name;
}

void Filter::setMode(Mode val)
{
    if (locked())
        return;

    _mode = val;
}

void Filter::setFilteringErrors(bool val)
{
    if (locked())
        return;

    _filteringErrors = val;
}

bool Filter::followThreadContext() const
{
    return _followThreadContext;
}

void Filter::setFollowThreadContext(bool val)
{
    if (locked())
        return;

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
                _threadContextIds[m.threadId] = m.timeSpec;

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
            _threadContextIds[m.threadId] = m.timeSpec;

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

    timespec curTime;

#if defined(__MINGW32__)
    clock_gettime(CLOCK_REALTIME, &curTime);
#else
    timespec_get(&curTime, TIME_UTC);
#endif

    vector<pid_t> tids;
    for (const auto& tci : _threadContextIds)
    {
        const timespec& timeSpec = tci.second;
        // Таймаут в 3 сек.
        if (curTime.tv_sec > (timeSpec.tv_sec + 3)
            || ((curTime.tv_sec == (timeSpec.tv_sec + 3))
                && (curTime.tv_nsec > timeSpec.tv_nsec)))
        {
            tids.push_back(tci.first);
        }
    }
    for (pid_t tid : tids)
        _threadContextIds.erase(tid);
}

//------------------------------- FilterModule -------------------------------

void FilterModule::addModule(const string& name)
{
    if (locked())
        return;

    if (_modules.findRef(name))
        return;

    _modules.addCopy(name);
    _modules.sort();
}

void FilterModule::setFilteringNoNameModules(bool val)
{
    if (locked())
        return;

    _filteringNoNameModules = val;
}

bool FilterModule::checkImpl(const Message& m) const
{
    if ((m.module == 0) && !_filteringNoNameModules)
        return true;

    lst::FindResult fr = _modules.find(m.module);
    bool res = fr.success();

    return (mode() == Mode::Exclude) ? !res : res;
}

//------------------------------- FilterLevel --------------------------------

void FilterLevel::setLevel(Level val)
{
    if (locked())
        return;

    _level = val;
}

bool FilterLevel::checkImpl(const Message& m) const
{
    if ((m.module == 0) && !filteringNoNameModules())
        return true;

    if (_level == None)
        return true;

    if (mode() == Mode::Include)
    {
        lst::FindResult fr = modules().find(m.module);
        if (fr.failed())
            return true;

        return (m.level <= _level);
    }

    // Для mode() == Mode::Exclude
    lst::FindResult fr = modules().find(m.module);
    if (fr.success())
        return true;

    return (m.level <= _level);
}

//-------------------------------- FilterFile --------------------------------

void FilterFile::addFile(const string& name)
{
    if (locked())
        return;

    string file;
    int line = 0;
    if (const char* d = strrchr(name.c_str(), ':'))
    {
        file = string(name.c_str(), size_t(d - name.c_str()));
        line = std::atoi(d + 1);
    }
    else
        file = name;

    FileLine* fl = _files.findItem(file.c_str());
    if (fl == 0)
    {
        fl = _files.add();
        fl->file = file;
        _files.sort();
    }
    if (line)
        fl->lines.insert(line);
}

bool FilterFile::checkImpl(const Message& m) const
{
    bool res = false;
    if (FileLine* fl = _files.findItem(m.file))
    {
        if (!fl->lines.empty())
            res = (fl->lines.find(m.line) != fl->lines.end());
        else
            res = true;
    }
    return (mode() == Mode::Exclude) ? !res : res;
}

//-------------------------------- FilterFunc --------------------------------

void FilterFunc::addFunc(const string& name)
{
    if (locked())
        return;

    if (_funcs.findRef(name))
        return;

    _funcs.addCopy(name);
    _funcs.sort();
}

bool FilterFunc::checkImpl(const Message& m) const
{
    lst::FindResult fr = _funcs.find(m.func);
    bool res = fr.success();

    return (mode() == Mode::Exclude) ? !res : res;
}

//------------------------------- FilterThread -------------------------------

bool FilterThread::followThreadContext() const
{
    return false;
}

void FilterThread::addThread(long id)
{
    if (locked())
        return;

    _threads.insert(pid_t(id));
}

bool FilterThread::checkImpl(const Message& m) const
{
    bool res = (_threads.find(m.threadId) != _threads.end());
    return (mode() == Mode::Exclude) ? !res : res;
}

//------------------------------ FilterContent -------------------------------

void FilterContent::addContent(const string& content)
{
    if (locked())
        return;

    if (_contents.findRef(content))
        return;

    _contents.addCopy(content);
    _contents.sort();
}

bool FilterContent::checkImpl(const Message& m) const
{
    bool res = false;
    for (const string* content : _contents)
        if (string::npos != m.str.find(*content))
        {
            res = true;
            break;
        }

    return (mode() == Mode::Exclude) ? !res : res;
}

//---------------------------------- Saver -----------------------------------

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
    SpinLocker locker {_filtersLock}; (void) locker;
    for (int i = 0; i < _filters.count(); ++i)
    {
        Filter* f = _filters.item(i);
        f->add_ref();
        filters.add(f);
    }
    return filters;
}

void Saver::addFilter(FilterPtr filter)
{
    SpinLocker locker {_filtersLock}; (void) locker;
    lst::FindResult fr = _filters.findRef(filter->name(), {lst::BruteForce::Yes});
    if (fr.success())
        _filters.remove(fr.index());

    filter->lock();
    filter->add_ref();
    _filters.add(filter.get());
}

void Saver::removeFilter(const string& name)
{
    SpinLocker locker {_filtersLock}; (void) locker;
    lst::FindResult fr = _filters.findRef(name, {lst::BruteForce::Yes});
    if (fr.success())
        _filters.remove(fr.index());
}

void Saver::clearFilters()
{
    SpinLocker locker {_filtersLock}; (void) locker;
    _filters.clear();
}

bool Saver::skipMessage(const Message& m, const FilterList& filters)
{
    if (filters.empty())
        return false;

    if (!_filtersActive)
        return false;

    for (Filter* filter : filters)
    {
        Filter::Check res = filter->check(m);
        if (res == Filter::Check::MessError)
        {
            // Прерываем фильтрацию на первом фильтре, который не фильтрует
            // сообщения об ошибках
            return false;
        }
        if (res == Filter::Check::Fail)
            return true;
    }

    // Если попали в эту точку - значит результат проверки последнего фильтра
    // равен Filter::Check::Success или Filter::Check::NoLock. В обоих случаях
    // сообщение не должно исключаться из вывода в лог-файл
    return false;
}

void Saver::removeIdsTimeoutThreads()
{
    FilterList filters = this->filters();
    for (Filter* filter : filters)
        filter->removeIdsTimeoutThreads();
}

//------------------------------- SaverStdOut --------------------------------

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
            (*_out) << m->prefix1;
            if (level() == Level::Debug2)
                (*_out) << m->prefix2;
            (*_out) << m->prefix3;
        }

        string str;
        string* pstr = &m->str;
        if (m->something && m->something->canModifyMessage())
        {
            str = m->something->modifyMessage(m->str);
            pstr = &str;
        }
        if ((maxLineSize() > 0) && (maxLineSize() < int(pstr->size())))
        {
            strncpy(&lineBuff[0], pstr->c_str(), maxLineSize());
            (*_out) << (char*) &lineBuff[0];
        }
        else
            (*_out) << *pstr;

        (*_out) << "\n";

        if (++flushCount % 50 == 0)
            _out->flush();
    }
    _out->flush();
}

//------------------------------- SaverStdErr --------------------------------

SaverStdErr::SaverStdErr(const char* name, Level level, bool shortMessages)
    : SaverStdOut(name, level, shortMessages)
{
    _out = &std::cerr;
}

//-------------------------------- SaverFile ---------------------------------

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

    FILE* f = fopen(_filePath.c_str(), "a");
    if (f == 0)
        throw std::logic_error("Could not open file: " + _filePath);

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

        fputs(m->prefix1, f);
        if (level() == Level::Debug2)
            fputs(m->prefix2, f);
        fputs(m->prefix3, f);

        string str;
        string* pstr = &m->str;
        if (m->something && m->something->canModifyMessage())
        {
            str = m->something->modifyMessage(m->str);
            pstr = &str;
        }
        if ((maxLineSize() > 0) && (maxLineSize() < int(pstr->size())))
        {
            strncpy(lineBuff.data(), pstr->c_str(), maxLineSize());
            fputs(lineBuff.data(), f);
        }
        else
            fputs(pstr->c_str(), f);

        fputs("\n", f);

        if (++flushCount % 500 == 0)
            fflush(f);
    }
    fflush(f);
    fclose(f);
}

//----------------------------------- Line -----------------------------------

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
        message->str = std::move(impl->buff);

#if defined(__MINGW32__)
        clock_gettime(CLOCK_REALTIME, &message->timeSpec);
#else
        timespec_get(&message->timeSpec, TIME_UTC);
#endif
        message->threadId = trd::gettid();
        message->something = std::move(impl->something);

        message->file = impl->file;
        message->func = impl->func;
        message->line = impl->line;
        message->module = impl->module;

        impl->logger->addMessage(std::move(message));

        if (impl->level == Error)
            impl->logger->flush();
    }
    catch (...)
    {}
}

//---------------------------------- Logger ----------------------------------

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
    SpinLocker locker {_messagesLock}; (void) locker;
    _messages.add(m.release());
}

void Logger::run()
{
    steady_timer flushTimer;
    MessageList messagesBuff;

    // Вспомогательный флаг,  нужен чтобы дать возможность  перед  прерыванием
    // потока сделать лишний цикл while (true) и сбросить все буферы в сэйверы.
    // Примечание: threadStop() для этой цели использовать нельзя
    bool loopBreak = false;

    auto saverFlush = [](const MessageList& messages, Saver* saver)
    {
        if (!saver->active())
            return;

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
        bool messagesIsEmpty;
        { //Block for SpinLocker
            SpinLocker locker {_messagesLock}; (void) locker;
            messagesIsEmpty = _messages.empty();
        }

        if (!threadStop() && messagesIsEmpty && (_flushLoop == 0))
        {
            static chrono::milliseconds sleepThread {20};
            this_thread::sleep_for(sleepThread);
        }

        MessageList messages;
        { //Block for SpinLocker
            SpinLocker locker {_messagesLock}; (void) locker;
            messages.swap(_messages);
        }
        if (!threadStop() && messages.empty() && messagesBuff.empty())
        {
            _flushLoop = 0;
            continue;
        }

        if (!messages.empty())
        {
            auto prefixFormatterL = [this](MessageList& messages, int min, int max)
            {
                time_t lastTime = 0;
                char prefix1Buff[sizeof(Message::prefix1)]; // = {0};

                // Значение для Message::prefix1 всегда фиксированной длины,
                // поэтому нет необходимости присваивать последний нуль
                // prefix1Buff[sizeof(prefix1Buff) - 1] = '\0';

                Level level = this->level(); // volatile оптимизация
                for (int i = min; i < max; ++i)
                {
                    prefixFormatter1(messages[i], lastTime, prefix1Buff);
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

            { //Block for SpinLocker
                SpinLocker locker {_saversLock}; (void) locker;
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

        } //if (!messages.empty())

        if (loopBreak
            || _flushLoop > 0
            || flushTimer.elapsed() > _flushTime
            || messagesBuff.count() > _flushSize)
        {
            flushTimer.reset();
            if (!messagesBuff.empty())
            {
                SaverList savers = this->savers(false);
                for (Saver* saver : savers)
                    saverFlush(messagesBuff, saver);
            }
            if (_flushLoop > 0)
                --_flushLoop;

            messagesBuff.clear();
        }
        if (loopBreak)
            break;

        if (threadStop())
            loopBreak = true;
    } //while (true)
}

void Logger::flush(int loop)
{
    _flushLoop = (loop < 1) ? 1 : loop;
}

void Logger::waitingFlush()
{
    while (_flushLoop && !threadStop())
    {
        static chrono::milliseconds sleepThread {10};
        this_thread::sleep_for(sleepThread);
    }
}

void Logger::addSaverStdOut(Level level, bool shortMessages)
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker {_saversLock}; (void) locker;
        _saverOut = SaverPtr(new SaverStdOut("stdout", level, shortMessages));
        _saverOut->setLogger(this);
    }
    redefineLevel();
}

void Logger::addSaverStdErr(Level level, bool shortMessages)
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker {_saversLock}; (void) locker;
        _saverErr = SaverPtr(new SaverStdErr("stderr", level, shortMessages));
        _saverErr->setLogger(this);
    }
    redefineLevel();
}

void Logger::removeSaverStdOut()
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker {_saversLock}; (void) locker;
        _saverOut.reset();
    }
    redefineLevel();
}

void Logger::removeSaverStdErr()
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker {_saversLock}; (void) locker;
        _saverErr.reset();
    }
    redefineLevel();
}

void Logger::addSaver(SaverPtr saver)
{
    { //Block for SpinLocker
        SpinLocker locker {_saversLock}; (void) locker;
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
        SpinLocker locker {_saversLock}; (void) locker;
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
    SaverList savers = this->savers(true);
    Saver* saver = savers.findItem(&name, {lst::BruteForce::Yes});
    return SaverPtr(saver);
}

void Logger::clearSavers(bool clearStd)
{
    { //Block for SpinLocker
        SpinLocker locker {_saversLock}; (void) locker;
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

SaverList Logger::savers(bool withStd) const
{
    SaverList savers;
    { //Block for SpinLocker
        SpinLocker locker {_saversLock}; (void) locker;
        for (Saver* s : _savers)
        {
            s->add_ref();
            savers.add(s);
        }
        if (withStd)
        {
            if (_saverOut)
            {
                _saverOut->add_ref();
                savers.add(_saverOut.get());
            }
            if (_saverErr)
            {
                _saverErr->add_ref();
                savers.add(_saverErr.get());
            }
        }
    }
    return savers;
}

void Logger::redefineLevel()
{
    Level level = None;
    SaverList savers = this->savers(true);
    for (Saver* saver : savers)
        if (saver->active() && (saver->level() > level))
            level = saver->level();

    _level = level;
}

Logger& logger()
{
    return safe::singleton<Logger>();
}

//------------------------------ Line operators ------------------------------

Line& operator<< (Line& line, bool b)
{
    if (line.toLogger())
        line.impl->buff += (b ? "true" : "false");
    return line;
}

Line& operator<< (Line& line, char c)
{
    if (line.toLogger())
        line.impl->buff += c;
    return line;
}

Line& operator<< (Line& line, char* c)
{
    if (line.toLogger() && c)
        line.impl->buff += c;
    return line;
}

Line& operator<< (Line& line, const char* c)
{
    if (line.toLogger() && c)
        line.impl->buff += c;
    return line;
}

Line& operator<< (Line& line, const string& s)
{
    if (line.toLogger())
        line.impl->buff += s;
    return line;
}

Line& operator<< (Line& line, const string* s)
{
    if (line.toLogger() && s)
        line.impl->buff += s->c_str();
    return line;
}

Line& operator<< (Line& line, const timespec& ts)
{
    if (line.toLogger())
    {
        // Отладить
        break_point

#if __cplusplus >= 201703L && !defined(LOGGER_USE_SNPRINTF)
        // tv_sec
        detail::stream_operator(line, ts.tv_sec);

        // tv_usec
        // При размере буфера в 8 символов функция usecToString() может вернуть
        // значение INVALID,  если по какой-то причине tv_usec окажется больше,
        // чем 6 знаков. Чтобы этого  избежать  размер  буфера  увеличен  до 16
        // символов
        char buff[16];
        long tv_usec = long(ts.tv_nsec / 1000);
        usecToString<sizeof(buff)>(tv_usec, buff);
        line.impl->buff += buff;
#else
        char buff[8];
        long tv_usec = long(ts.tv_nsec / 1000);
        snprintf(buff, sizeof(buff), ".%06ld", tv_usec);
        line.impl->buff += to_string(tv.tv_sec);
        line.impl->buff += buff;
#endif
    }
    return line;
}

const char* __file__cache(const char* file)
{
    static StringList list;
    static atomic_flag lock = ATOMIC_FLAG_INIT;

    const char* f = strrchr(file, '/');

    if (f)
        f += 1;
    else
        f = file;

    if (*f == '\0')
        return 0;

    SpinLocker locker {lock}; (void) locker;
    if (lst::FindResult fr = list.find(f))
        return list.item(fr.index())->c_str();

    f = list.add(new string(f))->c_str();
    list.sort();
    return f;
}

void stop()
{
    logger().flush();
    logger().waitingFlush();
    logger().stop();
}

string round(float value, int signCount)
{
    return round(double(value), signCount);
}

string round(double value, int signCount)
{
    char buff[30]; buff[sizeof(buff) - 1] = '\0';
    string fmt = "%." + to_string(signCount) + "lf";
    snprintf(buff, sizeof(buff) - 1, fmt.c_str(), value);
    return buff;
}

} // namespace alog
