#include "logger.h"
#include "break_point.h"
#include "spin_locker.h"
#include "simple_timer.h"
#include "utils.h"

#include <ctime>
#include <stdexcept>
#include <algorithm>
#include <string.h>
//#include <signal.h>


namespace alog
{
using namespace std;

// Функция записывает сообщения об ошибке произошедшей в самом логгере.
// Информация сохраняется в файле /tmp/alogger.log
void loggerPanic(const char* saverName, const char* error)
{
    if (FILE* f = fopen("/tmp/alogger.log", "a"))
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
        return Level::NONE;
    else if (level == "error")
        return Level::ERROR;
    else if (level == "warning")
        return Level::WARNING;
    else if (level == "info")
        return Level::INFO;
    else if (level == "verbose")
        return Level::VERBOSE;
    else if (level == "debug")
        return Level::DEBUG;
    else if (level == "debug2")
        return Level::DEBUG2;

    return Level::INFO;
}

static const char* levelToStringImpl(Level level)
{
    switch (level)
    {
        // Примечание: пробелы в конце строк удалять нельзя, так как
        // это скажется на производительности функции prefixFormatter2().
        case Level::NONE:    return "NONE    ";
        case Level::ERROR:   return "ERROR   ";
        case Level::WARNING: return "WARNING ";
        case Level::INFO:    return "INFO    ";
        case Level::VERBOSE: return "VERBOSE ";
        case Level::DEBUG:   return "DEBUG   ";
        case Level::DEBUG2:  return "DEBUG2  ";
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
void prefixFormatter(Message& message)
{
    char buff[sizeof(Message::prefix)] = {0};

    std::tm tm;
    memset(&tm, 0, sizeof(tm));

    time_t t = message.timeVal.tv_sec;
    localtime_r(&t, &tm);

//    //if (message.level == Level::DEBUG2)
//    if (logger->level() == Level::DEBUG2)
//    {
//        long tv_usec = long(message.timeVal.tv_usec);
//        snprintf(buff, sizeof(buff) - 1,
//                 "%02d.%02d.%04d %02d:%02d:%02d.%06ld ",
//                 tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, tv_usec);
//    }
//    else
//        snprintf(buff, sizeof(buff) - 1,
//                 "%02d.%02d.%04d %02d:%02d:%02d ",
//                 tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    snprintf(buff, sizeof(buff) - 1,
             "%02d.%02d.%04d %02d:%02d:%02d",
             tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    memcpy(message.prefix, buff, sizeof(buff));
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
        snprintf(module, sizeof(module) - 1, "%s : ", message.module.c_str());

    if (!message.file.empty())
        snprintf(buff, sizeof(buff) - 1, " %sLWP%ld [%s:%d:%s]\t%s",
                 level, tid, message.file.c_str(), message.line, message.func.c_str(), module);
    else
        snprintf(buff, sizeof(buff) - 1, " %sLWP%ld\t%s", level, tid, module);

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

    if ((m.level == ERROR) && !_filteringErrors)
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

    vector<pid_t> tids;
    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    for (const auto& tci : _threadContextIds)
    {
        // Таймаут в 3 сек.
        if (cur_time.tv_sec > (tci.second.tv_sec + 3)
            || ((cur_time.tv_sec == (tci.second.tv_sec + 3))
                && (cur_time.tv_usec > tci.second.tv_usec)))
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

    if (_level == NONE)
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

    if (_level == Level::NONE)
        return;

    flushImpl(messages);
}

FilterList Saver::filters() const
{
    FilterList filters_;
    SpinLocker locker(_filtersLock); (void) locker;
    for (int i = 0; i < _filters.count(); ++i)
    {
        Filter* f = _filters.item(i);
        f->add_ref();
        filters_.add(f);
    }
    return std::move(filters_);
}

void Saver::addFilter(FilterLPtr filter)
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

    vector<char> line_buff;
    if (maxLineSize() > 0)
    {
        line_buff.resize(maxLineSize() + 1);
        line_buff[maxLineSize()] = '\0';
    }

    unsigned flush_count = 0;
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
            if (level() == Level::DEBUG2)
                (*_out) << m->prefix2;
            (*_out) << m->prefix3;
        }
        if ((maxLineSize() > 0) && (maxLineSize() < int(m->str.size())))
        {
            strncpy(&line_buff[0], m->str.c_str(), maxLineSize());
            (*_out) << (char*) &line_buff[0];
        }
        else
            (*_out) << m->str;

        (*_out) << "\n";

        if (++flush_count % 50 == 0)
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

SaverFile::SaverFile(const string& name, const string& filePath, Level level)
    : Saver(name, level),
      _filePath(filePath)
{
}

void SaverFile::flushImpl(const MessageList& messages)
{
    if (messages.size() == 0)
        return;

    if (FILE* f = fopen(_filePath.c_str(),  "a"))
    {
        removeIdsTimeoutThreads();

        vector<char> line_buff;
        if (maxLineSize() > 0)
        {
            line_buff.resize(maxLineSize() + 1);
            line_buff[maxLineSize()] = '\0';
        }

        unsigned flush_count = 0;
        FilterList filters = this->filters();

        for (Message* m : messages)
        {
            if (m->level > level())
                continue;

            if (skipMessage(*m, filters))
                continue;

            fputs(m->prefix, f);
            if (level() == Level::DEBUG2)
                fputs(m->prefix2, f);
            fputs(m->prefix3, f);

            if ((maxLineSize() > 0) && (maxLineSize() < int(m->str.size())))
            {
                strncpy(&line_buff[0], m->str.c_str(), maxLineSize());
                fputs(&line_buff[0], f);
            }
            else
                fputs(m->str.c_str(), f);

            fputs("\n", f);

            if (++flush_count % 500 == 0)
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
    : impl(new Impl())
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
        MessagePtr message(new Message());
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

        if (impl->level == ERROR)
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
    simple_timer flush_timer;

    // Вспомогательный флаг, нужен чтобы дать возможность перед прерываением
    // потока сделать лишний цикл while (true) и сбросить все буферы в сэйверы.
    // Примечание: _threadStop для этой цели использовать нельзя.
    bool thread_stop = false;

    MessageList messages_buff;

    auto saver_flush = [] (const MessageList& messages, Saver* saver)
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
        int messages_count;
        { //Блок для SpinLocker
            SpinLocker locker(_messagesLock); (void) locker;
            messages_count = _messages.count();
        }

        if (!threadStop() && (messages_count == 0))
        {
            static chrono::milliseconds sleep_thread {20};
            this_thread::sleep_for(sleep_thread);
        }

        MessageList messages;
        { //Блок для SpinLocker
            SpinLocker locker(_messagesLock); (void) locker;
            messages.swap(_messages);
        }
        if (!threadStop()
            && messages.count() == 0
            && messages_buff.count() == 0)
        {
            _forceFlush = false;
            continue;
        }

        if (messages.count())
        {
            auto prefix_formatter = [this] (MessageList& messages, int min, int max)
            {
                Level level = this->level(); // volatile оптимизация
                for (int i = min; i < max; ++i)
                {
                    prefixFormatter(messages[i]);
                    if (level == Level::DEBUG2)
                        prefixFormatter2(messages[i]);
                    prefixFormatter3(messages[i]);
                }
            };

            int threads_count = 0;
            if (messages.count() > 50000)  ++threads_count;
            if (messages.count() > 100000) ++threads_count;
            if (messages.count() > 150000) ++threads_count;

            int step = messages.count();
            int thread_index = 0;
            vector<thread> threads;
            if (threads_count)
            {
                step = messages.count() / (threads_count + 1);
                for (; thread_index < threads_count; ++thread_index)
                {
                    threads.push_back(thread(prefix_formatter, std::ref(messages),
                                             thread_index * step, (thread_index + 1) * step));
                }
            }
            prefix_formatter(messages, thread_index * step, messages.count());

            for (size_t i = 0; i < threads.size(); ++i)
                threads[i].join();

            SaverLPtr saver_out;
            SaverLPtr saver_err;

            { //Блок для SpinLocker
                SpinLocker locker(_saversLock); (void) locker;
                saver_out = _saverOut;
                saver_err = _saverErr;
            }
            if (saver_out)
                saver_flush(messages, saver_out.get());
            if (saver_err)
                saver_flush(messages, saver_err.get());

            for (int i = 0; i < messages.count(); ++i)
                messages_buff.add(messages.release(i, lst::NO_COMPRESS_LIST));
            messages.clear();

        } //if (messages.count())

        if (thread_stop
            || _forceFlush
            || flush_timer.elapsed() > _flushTime
            || messages_buff.count() > _flushSize)
        {
            flush_timer.reset();
            if (messages_buff.count())
            {
                SaverList savers = this->savers();
                for (Saver* saver : savers)
                    saver_flush(messages_buff, saver);
            }
            _forceFlush = false;
            messages_buff.clear();
        }

        if (thread_stop)  break;
        if (threadStop()) thread_stop = true;

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
        static chrono::milliseconds sleep_thread {20};
        this_thread::sleep_for(sleep_thread);
    }
}

LevelProxy Logger::error_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, ERROR, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::warn_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, WARNING, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::info_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, INFO, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::verbose_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, VERBOSE, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::debug_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, DEBUG, file, func, line, module);
    return std::move(lp);
}

LevelProxy Logger::debug2_f(const char* file, const char* func, int line, const char* module)
{
    LevelProxy lp(this, DEBUG2, file, func, line, module);
    return std::move(lp);
}

void Logger::addSaverStdOut(Level level, bool shortMessages)
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        _saverOut = SaverLPtr(new SaverStdOut("stdout", level, shortMessages));
        _saverOut->setLogger(this);
    }
    redefineLevel();
}

void Logger::addSaverStdErr(Level level, bool shortMessages)
{
    waitingFlush();
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        _saverErr = SaverLPtr(new SaverStdErr("stderr", level, shortMessages));
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

void Logger::addSaver(SaverLPtr saver)
{
    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        lst::FindResult fr = _savers.findRef(saver->name(),
                                             lst::FindExtParams(lst::BruteForce::Yes));
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
        lst::FindResult fr = _savers.findRef(name, lst::FindExtParams(lst::BruteForce::Yes));
        if (fr.success())
        {
            _savers.item(fr.index())->setLogger(0);
            _savers.remove(fr.index());
        }
    }
    redefineLevel();
}

SaverLPtr Logger::findSaver(const string& name)
{
    SaverList savers = this->savers();
    Saver* saver = savers.findItem(&name, lst::FindExtParams(lst::BruteForce::Yes));
    return SaverLPtr(saver);
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
    Level level = NONE;
    SaverLPtr saver_out;
    SaverLPtr saver_err;

    { //Block for SpinLocker
        SpinLocker locker(_saversLock); (void) locker;
        saver_out = _saverOut;
        saver_err = _saverErr;
    }
    if (saver_out && saver_out->active() && (saver_out->level() > level))
        level = saver_out->level();
    if (saver_err && saver_err->active() && (saver_err->level() > level))
        level = saver_err->level();

    SaverList savers = this->savers();
    for (Saver* saver : savers)
        if (saver->active() && (saver->level() > level))
            level = saver->level();

    _level = level;
}

} // namespace alog
