/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2013 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "logger/config.h"
#include <stdexcept>

#define log_error_m   alog::logger().error   (alog_line_location, "LogConfig")
#define log_warn_m    alog::logger().warn    (alog_line_location, "LogConfig")
#define log_info_m    alog::logger().info    (alog_line_location, "LogConfig")
#define log_verbose_m alog::logger().verbose (alog_line_location, "LogConfig")
#define log_debug_m   alog::logger().debug   (alog_line_location, "LogConfig")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "LogConfig")

namespace alog {

using namespace std;

const char* yamlTypeName(YAML::NodeType::value type)
{
    switch (int(type))
    {
        case YAML::NodeType::Scalar:   return "Scalar";
        case YAML::NodeType::Sequence: return "Sequence";
        case YAML::NodeType::Map:      return "Map";
        default:                       return "Undefined";
    }
}

FilterPtr createFilter(const YAML::Node& yfilter)
{
    auto checkFiedType = [&yfilter](const string& field, YAML::NodeType::value type)
    {
        if (yfilter[field].IsNull())
            throw std::logic_error(
                "For 'filter' node a field '" + field + "' can not be null");

        if (yfilter[field].Type() != type)
            throw std::logic_error(
                "For 'filter' node a field '" + field + "' "
                "must have type '" + yamlTypeName(type) + "'");
    };

    string name;
    if (yfilter["name"].IsDefined())
    {
        checkFiedType("name", YAML::NodeType::Scalar);
        name = yfilter["name"].as<string>();
    }
    if (name.empty())
        throw std::logic_error("In a filter-node a field 'name' can not be empty");

    string type;
    if (yfilter["type"].IsDefined())
    {
        checkFiedType("type", YAML::NodeType::Scalar);
        type = yfilter["type"].as<string>();
    }
    if (type != "module_name"
        && type != "log_level"
        && type != "func_name"
        && type != "file_name"
        && type != "thread_id"
        && type != "content")
    {
        throw std::logic_error(
            "In a filter-node a field 'type' can take one of the following "
            "values: module_name, log_level, func_name, file_name, thread_id, "
            "content. "
            "Current value: " + type);
    }

    string mode = "include";
    if (yfilter["mode"].IsDefined())
    {
        checkFiedType("mode", YAML::NodeType::Scalar);
        mode = yfilter["mode"].as<string>();
    }
    if ( !(mode == "include" || mode == "exclude"))
        throw std::logic_error("In a filter-node a field 'mode' can take "
                               "the values: 'include' or 'exclude'");

    bool filteringErrors = false;
    if (yfilter["filtering_errors"].IsDefined())
    {
        checkFiedType("filtering_errors", YAML::NodeType::Scalar);
        filteringErrors = yfilter["filtering_errors"].as<bool>();
    }

    bool followThreadContext = false;
    if (yfilter["follow_thread_context"].IsDefined())
    {
        checkFiedType("follow_thread_context", YAML::NodeType::Scalar);
        followThreadContext = yfilter["follow_thread_context"].as<bool>();
    }

    bool filteringNonameModules = false;
    if (yfilter["filtering_noname_modules"].IsDefined())
    {
        checkFiedType("filtering_noname_modules", YAML::NodeType::Scalar);
        filteringNonameModules = yfilter["filtering_noname_modules"].as<bool>();
    }

    string logLevel = "info";
    if (yfilter["level"].IsDefined())
    {
        checkFiedType("level", YAML::NodeType::Scalar);
        logLevel = yfilter["level"].as<string>();
    }

    set<string> modules;
    if (yfilter["modules"].IsDefined())
    {
        checkFiedType("modules", YAML::NodeType::Sequence);
        const YAML::Node& ymodules = yfilter["modules"];
        for (const YAML::Node& ymodule : ymodules)
            modules.insert(ymodule.as<string>());
    }

    set<string> functions;
    if (yfilter["functions"].IsDefined())
    {
        checkFiedType("functions", YAML::NodeType::Sequence);
        const YAML::Node& yfunctions = yfilter["functions"];
        for (const YAML::Node& yfunction : yfunctions)
            functions.insert(yfunction.as<string>());
    }

    set<string> files;
    if (yfilter["files"].IsDefined())
    {
        checkFiedType("files", YAML::NodeType::Sequence);
        const YAML::Node& yfiles = yfilter["files"];
        for (const YAML::Node& yfile : yfiles)
            files.insert(yfile.as<string>());
    }

    set<long> threads;
    if (yfilter["threads"].IsDefined())
    {
        checkFiedType("threads", YAML::NodeType::Sequence);
        const YAML::Node& ythreads = yfilter["threads"];
        for (const YAML::Node& ythread : ythreads)
            threads.insert(ythread.as<long>());
    }

    set<string> contents;
    if (yfilter["contents"].IsDefined())
    {
        checkFiedType("contents", YAML::NodeType::Sequence);
        const YAML::Node& ycontents = yfilter["contents"];
        for (const YAML::Node& ycont : ycontents)
            contents.insert(ycont.as<string>());
    }

    FilterPtr filter;
    if (type == "module_name")
    {
        FilterModulePtr filterMod {new FilterModule};
        filterMod->setFilteringNoNameModules(filteringNonameModules);

        for (const string& module : modules)
            filterMod->addModule(module);

        filter = filterMod;
    }
    else if (type == "log_level")
    {
        FilterLevelPtr filterLevel {new FilterLevel};
        filterLevel->setFilteringNoNameModules(filteringNonameModules);
        filterLevel->setLevel(levelFromString(logLevel));

        for (const string& module : modules)
            filterLevel->addModule(module);

        filter = filterLevel;
    }
    else if (type == "func_name")
    {
        FilterFuncPtr filterFunc {new FilterFunc};
        for (const string& function : functions)
            filterFunc->addFunc(function);

        filter = filterFunc;
    }
    else if (type == "file_name")
    {
        FilterFilePtr filterFile {new FilterFile};
        for (const string& file : files)
            filterFile->addFile(file);

        filter = filterFile;
    }
    else if (type == "thread_id")
    {
        FilterThreadPtr filterThread {new FilterThread};
        for (long tid : threads)
            filterThread->addThread(tid);

        filter = filterThread;
    }
    else if (type == "content")
    {
        FilterContentPtr filterCont {new FilterContent};
        for (const string& cont : contents)
            filterCont->addContent(cont);

        filter = filterCont;
    }
    if (filter.empty())
        return FilterPtr();

    filter->setName(name);
    filter->setMode((mode == "include") ? Filter::Mode::Include : Filter::Mode::Exclude);
    filter->setFilteringErrors(filteringErrors);
    filter->setFollowThreadContext(followThreadContext);

    return filter;
}

SaverPtr createSaver(const YAML::Node& ysaver, const FilterList& filters)
{
    auto checkFiedType = [&ysaver](const string& field, YAML::NodeType::value type)
    {
        if (ysaver[field].IsNull())
            throw std::logic_error(
                "For 'saver' node a field '" + field + "' can not be null");

        if (ysaver[field].Type() != type)
            throw std::logic_error(
                "For 'saver' node a field '" + field + "' "
                "must have type '" + yamlTypeName(type) + "'");
    };

    string name;
    if (ysaver["name"].IsDefined())
    {
        checkFiedType("name", YAML::NodeType::Scalar);
        name = ysaver["name"].as<string>();
    }
    if (name.empty())
        throw std::logic_error("In a saver-node a field 'name' can not be empty");

    int active = -1;
    if (ysaver["active"].IsDefined())
    {
        checkFiedType("active", YAML::NodeType::Scalar);
        active = ysaver["active"].as<bool>();
    }

    string logLevel = "info";
    if (ysaver["level"].IsDefined())
    {
        checkFiedType("level", YAML::NodeType::Scalar);
        logLevel = ysaver["level"].as<string>();
    }

    int maxLineSize = -1;
    if (ysaver["max_line_size"].IsDefined())
    {
        checkFiedType("max_line_size", YAML::NodeType::Scalar);
        maxLineSize = ysaver["max_line_size"].as<int>();
    }

    string file;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
    if (ysaver["file_win"].IsDefined())
    {
        checkFiedType("file_win", YAML::NodeType::Scalar);
        file = ysaver["file_win"].as<string>();
    }
    const char* programData = "ProgramData";
    if (file.find(programData, 0) == 0)
    {
        const char* prdata = getenv("PROGRAMDATA");
        file.replace(0, strlen(programData), prdata);
        for (string::value_type& c : file)
            if (c == '\\') c = '/';
    }
#else
    if (ysaver["file"].IsDefined())
    {
        checkFiedType("file", YAML::NodeType::Scalar);
        file = ysaver["file"].as<string>();
    }
#endif
    if (file.empty())
        throw std::logic_error("In a saver-node a field 'file' can not be empty");

    if (file[0] == '~')
    {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
        const char* home = getenv("USERPROFILE");
#else
        const char* home = getenv("HOME");
#endif
        file.replace(0, 1, home);
        for (string::value_type& c : file)
            if (c == '\\') c = '/';
    }

    bool isContinue = true;
    if (ysaver["continue"].IsDefined())
    {
        checkFiedType("continue", YAML::NodeType::Scalar);
        isContinue = ysaver["continue"].as<bool>();
    }

    list<string> filterNames;
    if (ysaver["filters"].IsDefined())
    {
        checkFiedType("filters", YAML::NodeType::Sequence);
        const YAML::Node& yfilters = ysaver["filters"];
        for (const YAML::Node& yfilter : yfilters)
            filterNames.push_back(yfilter.as<string>());
    }

    Level level = levelFromString(logLevel);
    SaverPtr saver {new SaverFile(name, file, level, isContinue)};

    if (active >= 0)
        saver->setActive(active);
    if (maxLineSize >= 0)
        saver->setMaxLineSize(maxLineSize);

    for (const string& filterName : filterNames)
    {
        bool found = false;
        for (Filter* filter : filters)
            if (filter->name() == filterName)
            {
                saver->addFilter(FilterPtr(filter));
                found = true;
                break;
            }

        if (!found)
            throw std::logic_error(
                "For a saver-node impossible to assign filter "
                "with name '" + filterName + "'. Filter not found");
    }

    return saver;
}

bool loadFilters(const YAML::Node& filtersNode, FilterList& filters,
                 const string& confFile)
{
    bool result = false;
    try
    {
        if (!filtersNode.IsDefined())
            throw std::logic_error("Filters node is undefined");

        if (filtersNode.IsNull())
            return true;

        if (!filtersNode.IsSequence())
            throw std::logic_error("Filters node must have sequence type");

        for (const YAML::Node& yfilter : filtersNode)
            if (FilterPtr f = createFilter(yfilter))
                filters.add(f.detach());

        result = true;
    }
    catch (YAML::ParserException& e)
    {
        filters.clear();
        log_error_m << "Logger configuration YAML error. Detail: " << e.what()
                    << ". Config file: " << confFile;
    }
    catch (std::exception& e)
    {
        filters.clear();
        log_error_m << "Logger configuration error. Detail: " << e.what()
                    << ". Config file: " << confFile;
    }
    catch (...)
    {
        filters.clear();
        log_error_m << "Logger configuration unknown error"
                    << ". Config file: " << confFile;
    }
    return result;
}

bool loadSavers(const string& confFile, SaverList& savers)
{
    bool result = false;
    try
    {
        YAML::Node conf = YAML::LoadFile(confFile);

        const YAML::Node& yfilters = conf["filters"];
        FilterList filters;
        loadFilters(yfilters, filters, confFile);

        const YAML::Node& ysavers = conf["savers"];

        if (!ysavers.IsDefined())
            throw std::logic_error("Savers node is undefined");

        if (ysavers.IsNull())
            return true;

        if (!ysavers.IsSequence())
            throw std::logic_error("Savers node must have sequence type");

        for (const YAML::Node& ysaver : ysavers)
            if (SaverPtr s = createSaver(ysaver, filters))
                savers.add(s.detach());

        result = true;
    }
    catch (YAML::ParserException& e)
    {
        log_error_m << "Logger configuration YAML error. Detail: " << e.what()
                    << ". Config file: " << confFile;
    }
    catch (std::exception& e)
    {
        log_error_m << "Logger configuration error. Detail: " << e.what()
                    << ". Config file: " << confFile;
    }
    catch (...)
    {
        log_error_m << "Logger configuration unknown error"
                    << ". Config file: " << confFile;
    }
    return result;
}

bool loadSavers(const string& confFile)
{
    SaverList savers;
    if (alog::loadSavers(confFile, savers))
    {
        for (Saver* saver : savers)
            logger().addSaver(SaverPtr(saver));
        return true;
    }
    return false;
}

void printSaversInfo()
{
    log_info_m << "---";
    SaverList savers = alog::logger().savers();

    // Отключаем фильтрацию для default-сэйвера
    bool defaultFiltersActive = true;
    if (lst::FindResult fr = savers.findRef(string("default")))
    {
        defaultFiltersActive = savers.item(fr.index())->filtersActive();
        savers.item(fr.index())->setFiltersActive(false);
    }

    bool nextCommaVal;
    auto nextComma = [&nextCommaVal]()
    {
        if (nextCommaVal)
            return ", ";

        nextCommaVal = true;
        return "";
    };

    bool nextCommaVal2;
    auto nextComma2 = [&nextCommaVal2]()
    {
        if (nextCommaVal2)
            return ", ";

        nextCommaVal2 = true;
        return "";
    };

    for (Saver* saver : savers)
    {
        alog::Line logLine = log_info_m << "Saver : ";
        logLine << "name: " << saver->name()
                << "; active: " << saver->active()
                << "; level: " << levelToString(saver->level())
                << "; max_line_size: " << saver->maxLineSize();

        FilterList filters = saver->filters();
        logLine << "; filters: [";
        nextCommaVal = false;
        for (Filter* filter : filters)
            logLine << nextComma() << (filter->name().empty() ? string("''") : filter->name());
        logLine << "]";

        if (SaverFile* fsaver = dynamic_cast<SaverFile*>(saver))
            logLine << "; file: " << fsaver->filePath();
    }

    // Составляем список фильтров
    FilterList filters;
    for (Saver* saver : savers)
    {
        FilterList saverFilters = saver->filters();
        for (Filter* filter : saverFilters)
        {
            lst::FindResult fr = filters.findRef(filter->name());
            if (fr.success())
                continue;

            filter->add_ref();
            filters.add(filter);
        }
    }

    for (Filter* filter : filters)
    {
        alog::Line logLine = log_info_m << "Filter : ";
        logLine << "name: " << filter->name()
                << "; mode: " << ((filter->mode() == Filter::Mode::Include) ? "include" : "exclude")
                << "; filtering_errors: " << filter->filteringErrors()
                << "; follow_thread_context: " << filter->followThreadContext();

        nextCommaVal = false;
        if (FilterModule* modFilter = dynamic_cast<FilterModule*>(filter))
        {
            logLine << "; type: module_name"
                    << "; filtering_noname_modules: " << modFilter->filteringNoNameModules()
                    << "; modules: [";
            for (const string* module : modFilter->modules())
                logLine << nextComma() << module;
            logLine << "]";
        }
        else if (FilterLevel* logFilter = dynamic_cast<FilterLevel*>(filter))
        {
            logLine << "; type: log_level"
                    << "; level: " << levelToString(logFilter->leve());
        }
        else if (FilterFunc* funcFilter = dynamic_cast<FilterFunc*>(filter))
        {
            logLine << "; type: func_name"
                    << "; functions: [";
            for (const string* function : funcFilter->funcs())
                logLine << nextComma() << function;
            logLine << "]";
        }
        else if (FilterFile* fileFilter = dynamic_cast<FilterFile*>(filter))
        {
            logLine << "; type: file_name"
                    << "; files: [";
            for (FilterFile::FileLine* fl : fileFilter->files())
            {
                logLine << nextComma() << fl->file;
                if (!fl->lines.empty())
                {
                    logLine << ": [";
                    nextCommaVal2 = false;
                    for (int line : fl->lines)
                        logLine << nextComma2() << line;
                    logLine << "]";
                }
            }
            logLine << "]";
        }
        else if (FilterThread* threadFilter = dynamic_cast<FilterThread*>(filter))
        {
            logLine << "; type: thread_id"
                    << "; threads: [";
            for (pid_t tid : threadFilter->threads())
                logLine << nextComma() << long(tid);
            logLine << "]";
        }
        else if (FilterContent* contFilter = dynamic_cast<FilterContent*>(filter))
        {
            logLine << "; type: content"
                    << "; contents: [";
            for (const string* content : contFilter->contents())
                logLine << nextComma() << content;
            logLine << "]";
        }
    }
    log_info_m << "...";

    logger().flush(2);
    logger().waitingFlush();

    // Включаем фильтрацию для default-сэйвера
    if (lst::FindResult fr = savers.findRef(string("default")))
        savers.item(fr.index())->setFiltersActive(defaultFiltersActive);
}

} // namespace alog
