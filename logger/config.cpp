/* clang-format off */
#include "config.h"
#include "utils.h"

#include <yaml-cpp/yaml.h>
#include <exception>

#define log_error_m   alog::logger().error_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "LogConfig")
#define log_warn_m    alog::logger().warn_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "LogConfig")
#define log_info_m    alog::logger().info_f   (__FILE__, LOGGER_FUNC_NAME, __LINE__, "LogConfig")
#define log_verbose_m alog::logger().verbose_f(__FILE__, LOGGER_FUNC_NAME, __LINE__, "LogConfig")
#define log_debug_m   alog::logger().debug_f  (__FILE__, LOGGER_FUNC_NAME, __LINE__, "LogConfig")
#define log_debug2_m  alog::logger().debug2_f (__FILE__, LOGGER_FUNC_NAME, __LINE__, "LogConfig")


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
                "For filter-node a field '" + field + "' can not be null");

        if (yfilter[field].Type() != type)
            throw std::logic_error(
                "For filter-node a field '" + field + "' "
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
    if ( !(type == "module_name" || type == "log_level"))
        throw std::logic_error("In a filter-node a field 'type' can take "
                               "the values: 'module_name' or 'log_level'");

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

    FilterPtr filter;
    if (type == "module_name")
    {
        FilterModulePtr filterMod {new FilterModule()};
        filterMod->setFilteringNoNameModules(filteringNonameModules);

        for (const string& module : modules)
            filterMod->addModule(module);

        filter = filterMod;
    }
    else if (type == "log_level")
    {
        FilterLevelPtr filterLevel {new alog::FilterLevel()};
        filterLevel->setFilteringNoNameModules(filteringNonameModules);
        filterLevel->setLevel(levelFromString(logLevel));

        for (const string& module : modules)
            filterLevel->addModule(module);

        filter = filterLevel;
    }
    else if (type == "func_name")
    {
        FilterFuncPtr filterFunc {new FilterFunc()};
        for (const string& function : functions)
            filterFunc->addFunc(function);

        filter = filterFunc;
    }
    else if (type == "file_name")
    {
        FilterFilePtr filterFile {new FilterFile()};
        for (const string& file : files)
            filterFile->addFile(file);

        filter = filterFile;
    }
    else if (type == "thread_id")
    {
        FilterThreadPtr filterThread {new FilterThread()};
        for (long tid : threads)
            filterThread->addThread(tid);

        filter = filterThread;
    }
    if (filter.empty())
        return FilterPtr();

    filter->setName(name);
    filter->setMode((mode == "include") ? Filter::Mode::Include : Filter::Mode::Exclude);
    filter->setFilteringErrors(filteringErrors);
    filter->setFollowThreadContext(followThreadContext);

    return filter;
}

SaverPtr createSaver(const YAML::Node& ysaver, const list<FilterPtr>& filters)
{
    auto checkFiedType = [&ysaver](const string& field, YAML::NodeType::value type)
    {
        if (ysaver[field].IsNull())
            throw std::logic_error(
                "For saver-node a field '" + field + "' can not be null");

        if (ysaver[field].Type() != type)
            throw std::logic_error(
                "For saver-node a field '" + field + "' "
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
    if (ysaver["file"].IsDefined())
    {
        checkFiedType("file", YAML::NodeType::Scalar);
        file = ysaver["file"].as<string>();
    }
#if defined(_MSC_VER) || defined(__MINGW32__)
    if (ysaver["file_win"].IsDefined())
    {
        checkFiedType("file_win", YAML::NodeType::Scalar);
        file = ysaver["file_win"].as<string>();
    }
#endif
    if (file.empty())
        throw std::logic_error("In a saver-node a field 'file' can not be empty");

    if (file[0] == '~')
    {
#if defined(_MSC_VER) || defined(__MINGW32__)
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

    list<string> filters_;
    if (ysaver["filters"].IsDefined())
    {
        checkFiedType("filters", YAML::NodeType::Sequence);
        const YAML::Node& yfilters = ysaver["filters"];
        for (const YAML::Node& yfilter : yfilters)
            filters_.push_back(yfilter.as<string>());
    }

    Level level = levelFromString(logLevel);
    SaverPtr saver {new SaverFile(name, file, level, isContinue)};

    if (active >= 0)
        saver->setActive(active);
    if (maxLineSize >= 0)
        saver->setMaxLineSize(maxLineSize);

    for (const string& filter_ : filters_)
    {
        bool found = false;
        for (FilterPtr filter : filters)
            if (filter->name() == filter_)
            {
                saver->addFilter(filter);
                found = true;
                break;
            }

        if (!found)
            throw std::logic_error(
                "For a saver-node impossible to assign filter "
                "with name '" + filter_+ "'. Filter not found.");
    }

    return saver;
}

bool loadSavers(const string& confFile, std::list<SaverPtr>& savers)
{
    bool result = false;
    try
    {
        YAML::Node conf = YAML::LoadFile(confFile);

        list<FilterPtr> filters;
        const YAML::Node& yfilters = conf["filters"];
        if (yfilters.IsDefined())
        {
            if (!yfilters.IsSequence())
                throw std::logic_error("Filters-node must have sequence type");

            for (const YAML::Node& yfilter : yfilters)
                if (FilterPtr f = createFilter(yfilter))
                    filters.push_back(f);
        }

        const YAML::Node& ysavers = conf["savers"];
        if (ysavers.IsDefined())
        {
            if (!ysavers.IsSequence())
                throw std::logic_error("Savers-node must have sequence type");

            for (const YAML::Node& ysaver : ysavers)
                if (SaverPtr s = createSaver(ysaver, filters))
                    savers.push_back(s);
        }
        result = true;
    }
    catch (YAML::ParserException& e)
    {
        log_error_m << "YAML error. Detail: " << e.what()
                    << ". Config file: " << confFile;
    }
    catch (std::exception& e)
    {
        log_error_m << "YAML error. Detail: " << e.what()
                    << ". Config file: " << confFile;
    }
    catch (...)
    {
        log_error_m << "Unknown error"
                    << ". Config file: " << confFile;
    }
    return result;
}

void printSaversInfo()
{
    log_info_m << "---";
    SaverList savers = alog::logger().savers();
    for (Saver* saver : savers)
        if (SaverFile* fsaver = dynamic_cast<SaverFile*>(saver))
        {
            alog::Line logLine = log_info_m << "Saver : ";
            logLine << "name: " << fsaver->name() << "; "
                     << "active: " << fsaver->active() << "; "
                     << "level: " << levelToString(fsaver->level()) << "; "
                     << "max_line_size: " << fsaver->maxLineSize() << "; ";

            FilterList filters = fsaver->filters();
            logLine << "filters: [ ";
            for (Filter* filter : filters)
                logLine << (filter->name().empty() ? string("''") : filter->name()) << ", ";
            logLine << "]; ";
            logLine << "file: " << fsaver->filePath();
        }

    // Составляем список фильтров
    FilterList filters;
    for (Saver* saver : savers)
    {
        FilterList saverFilters = saver->filters();
        for (Filter* filter : saverFilters)
        {
            lst::FindResult fr = filters.findRef(filter->name(),
                                                 lst::FindExtParams(lst::BruteForce::Yes));
            if (fr.success())
                continue;

            filter->add_ref();
            filters.add(filter);
        }
    }

    for (Filter* filter : filters)
    {
        alog::Line logLine = log_info_m << "Filter : ";
        logLine << "name: " << filter->name() << "; "
                 << "mode: " << ((filter->mode() == Filter::Mode::Include) ? "include" : "exclude") << "; "
                 << "filtering_errors: " << filter->filteringErrors() << "; "
                 << "follow_thread_context: " << filter->followThreadContext() << "; ";

        if (FilterModule* modFilter = dynamic_cast<FilterModule*>(filter))
        {
            logLine << "type: module_name"
                     << "filtering_noname_modules: " << modFilter->filteringNoNameModules() << "; ";
            logLine << "modules: [ ";
            for (const string& module : modFilter->modules())
                logLine << module << ", ";
            logLine << "]; ";
        }
        else if (FilterLevel* logFilter = dynamic_cast<FilterLevel*>(filter))
        {
            logLine << "type: log_level";
            logLine << "level: " << levelToString(logFilter->leve()) << "; ";
        }
        else if (FilterFunc* funcFilter = dynamic_cast<FilterFunc*>(filter))
        {
            logLine << "type: func_name";
            logLine << "functions: [ ";
            for (const string& function : funcFilter->funcs())
                logLine << function << ", ";
            logLine << "]; ";
        }
        else if (FilterFile* fileFilter = dynamic_cast<FilterFile*>(filter))
        {
            logLine << "type: file_name";
            logLine << "files: [ ";
            for (const string& file : fileFilter->files())
                logLine << file << ", ";
            logLine << "]; ";
        }
        else if (FilterThread* threadFilter = dynamic_cast<FilterThread*>(filter))
        {
            logLine << "type: thread_id";
            logLine << "threads: [ ";
            for (pid_t tid : threadFilter->threads())
                logLine << long(tid) << ", ";
            logLine << "]; ";
        }
    }
    log_info_m << "...";
}

} // namespace alog

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
