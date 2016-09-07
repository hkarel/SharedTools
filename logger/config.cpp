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
    auto check_fied_type = [&yfilter](const string& field, YAML::NodeType::value type)
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
        check_fied_type("name", YAML::NodeType::Scalar);
        name = yfilter["name"].as<string>();
    }
    if (name.empty())
        throw std::logic_error("In a filter-node a field 'name' can not be empty");

    string type;
    if (yfilter["type"].IsDefined())
    {
        check_fied_type("type", YAML::NodeType::Scalar);
        type = yfilter["type"].as<string>();
    }
    if ( !(type == "module_name" || type == "log_level"))
        throw std::logic_error("In a filter-node a field 'type' can take "
                               "the values: 'module_name' or 'log_level'");

    string mode = "include";
    if (yfilter["mode"].IsDefined())
    {
        check_fied_type("mode", YAML::NodeType::Scalar);
        mode = yfilter["mode"].as<string>();
    }
    if ( !(mode == "include" || mode == "exclude"))
        throw std::logic_error("In a filter-node a field 'mode' can take "
                               "the values: 'include' or 'exclude'");

    bool filtering_errors = false;
    if (yfilter["filtering_errors"].IsDefined())
    {
        check_fied_type("filtering_errors", YAML::NodeType::Scalar);
        filtering_errors = yfilter["filtering_errors"].as<bool>();
    }

    bool follow_thread_context = false;
    if (yfilter["follow_thread_context"].IsDefined())
    {
        check_fied_type("follow_thread_context", YAML::NodeType::Scalar);
        follow_thread_context = yfilter["follow_thread_context"].as<bool>();
    }

    bool filtering_noname_modules = false;
    if (yfilter["filtering_noname_modules"].IsDefined())
    {
        check_fied_type("filtering_noname_modules", YAML::NodeType::Scalar);
        filtering_noname_modules = yfilter["filtering_noname_modules"].as<bool>();
    }

    string log_level = "info";
    if (yfilter["level"].IsDefined())
    {
        check_fied_type("level", YAML::NodeType::Scalar);
        log_level = yfilter["level"].as<string>();
    }

    set<string> modules;
    if (yfilter["modules"].IsDefined())
    {
        check_fied_type("modules", YAML::NodeType::Sequence);
        const YAML::Node& ymodules = yfilter["modules"];
        for (const YAML::Node& ymodule : ymodules)
            modules.insert(ymodule.as<string>());
    }

    set<string> functions;
    if (yfilter["functions"].IsDefined())
    {
        check_fied_type("functions", YAML::NodeType::Sequence);
        const YAML::Node& yfunctions = yfilter["functions"];
        for (const YAML::Node& yfunction : yfunctions)
            functions.insert(yfunction.as<string>());
    }

    set<string> files;
    if (yfilter["files"].IsDefined())
    {
        check_fied_type("files", YAML::NodeType::Sequence);
        const YAML::Node& yfiles = yfilter["files"];
        for (const YAML::Node& yfile : yfiles)
            files.insert(yfile.as<string>());
    }

    set<long> threads;
    if (yfilter["threads"].IsDefined())
    {
        check_fied_type("threads", YAML::NodeType::Sequence);
        const YAML::Node& ythreads = yfilter["threads"];
        for (const YAML::Node& ythread : ythreads)
            threads.insert(ythread.as<long>());
    }

    FilterPtr filter;
    if (type == "module_name")
    {
        FilterModulePtr filter_mod {new FilterModule()};
        filter_mod->setFilteringNoNameModules(filtering_noname_modules);

        for (const string& module : modules)
            filter_mod->addModule(module);

        filter = filter_mod;
    }
    else if (type == "log_level")
    {
        FilterLevelPtr filter_level {new alog::FilterLevel()};
        filter_level->setFilteringNoNameModules(filtering_noname_modules);
        filter_level->setLevel(levelFromString(log_level));

        for (const string& module : modules)
            filter_level->addModule(module);

        filter = filter_level;
    }
    else if (type == "func_name")
    {
        FilterFuncPtr filter_func {new FilterFunc()};
        for (const string& function : functions)
            filter_func->addFunc(function);

        filter = filter_func;
    }
    else if (type == "file_name")
    {
        FilterFilePtr filter_file {new FilterFile()};
        for (const string& file : files)
            filter_file->addFile(file);

        filter = filter_file;
    }
    else if (type == "thread_id")
    {
        FilterThreadPtr filter_thread {new FilterThread()};
        for (long tid : threads)
            filter_thread->addThread(tid);

        filter = filter_thread;
    }
    if (filter.empty())
        return FilterPtr();

    filter->setName(name);
    filter->setMode((mode == "include") ? Filter::Mode::Include : Filter::Mode::Exclude);
    filter->setFilteringErrors(filtering_errors);
    filter->setFollowThreadContext(follow_thread_context);

    return filter;
}

SaverPtr createSaver(const YAML::Node& ysaver, const list<FilterPtr>& filters)
{
    auto check_fied_type = [&ysaver](const string& field, YAML::NodeType::value type)
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
        check_fied_type("name", YAML::NodeType::Scalar);
        name = ysaver["name"].as<string>();
    }
    if (name.empty())
        throw std::logic_error("In a saver-node a field 'name' can not be empty");

    int active = -1;
    if (ysaver["active"].IsDefined())
    {
        check_fied_type("active", YAML::NodeType::Scalar);
        active = ysaver["active"].as<bool>();
    }

    string log_level = "info";
    if (ysaver["level"].IsDefined())
    {
        check_fied_type("level", YAML::NodeType::Scalar);
        log_level = ysaver["level"].as<string>();
    }

    int max_line_size = -1;
    if (ysaver["max_line_size"].IsDefined())
    {
        check_fied_type("max_line_size", YAML::NodeType::Scalar);
        max_line_size = ysaver["max_line_size"].as<int>();
    }

    string file;
    if (ysaver["file"].IsDefined())
    {
        check_fied_type("file", YAML::NodeType::Scalar);
        file = ysaver["file"].as<string>();
    }
    if (file.empty())
        throw std::logic_error("In a saver-node a field 'file' can not be empty");

    if (file[0] == '~')
    {
#if defined(_MSC_VER) || defined(__MINGW32__)
        // Отладить под Windows
        break_point
#endif
        const char* home = getenv("HOME");
        file.replace(0, 1, home);
    }

    bool is_continue = true;
    if (ysaver["continue"].IsDefined())
    {
        check_fied_type("continue", YAML::NodeType::Scalar);
        is_continue = ysaver["continue"].as<bool>();
    }

    list<string> filters_;
    if (ysaver["filters"].IsDefined())
    {
        check_fied_type("filters", YAML::NodeType::Sequence);
        const YAML::Node& yfilters = ysaver["filters"];
        for (const YAML::Node& yfilter : yfilters)
            filters_.push_back(yfilter.as<string>());
    }

    Level level = levelFromString(log_level);
    SaverPtr saver {new SaverFile(name, file, level, is_continue)};

    if (active >= 0)
        saver->setActive(active);
    if (max_line_size >= 0)
        saver->setMaxLineSize(max_line_size);

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
            alog::Line log_line = log_info_m << "Saver : ";
            log_line << "name: " << fsaver->name() << "; "
                     << "active: " << fsaver->active() << "; "
                     << "level: " << levelToString(fsaver->level()) << "; "
                     << "max_line_size: " << fsaver->maxLineSize() << "; ";

            FilterList filters = fsaver->filters();
            log_line << "filters: [ ";
            for (Filter* filter : filters)
                log_line << (filter->name().empty() ? string("''") : filter->name()) << ", ";
            log_line << "]; ";
            log_line << "file: " << fsaver->filePath();
        }

    // Составляем список фильтров
    FilterList filters;
    for (Saver* saver : savers)
    {
        FilterList saver_filters = saver->filters();
        for (Filter* filter : saver_filters)
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
        alog::Line log_line = log_info_m << "Filter : ";
        log_line << "name: " << filter->name() << "; "
                 << "mode: " << ((filter->mode() == Filter::Mode::Include) ? "include" : "exclude") << "; "
                 << "filtering_errors: " << filter->filteringErrors() << "; "
                 << "follow_thread_context: " << filter->followThreadContext() << "; ";

        if (FilterModule* mod_filter = dynamic_cast<FilterModule*>(filter))
        {
            log_line << "type: module_name"
                     << "filtering_noname_modules: " << mod_filter->filteringNoNameModules() << "; ";
            log_line << "modules: [ ";
            for (const string& module : mod_filter->modules())
                log_line << module << ", ";
            log_line << "]; ";
        }
        else if (FilterLevel* log_filter = dynamic_cast<FilterLevel*>(filter))
        {
            log_line << "type: log_level";
            log_line << "level: " << levelToString(log_filter->leve()) << "; ";
        }
        else if (FilterFunc* func_filter = dynamic_cast<FilterFunc*>(filter))
        {
            log_line << "type: func_name";
            log_line << "functions: [ ";
            for (const string& function : func_filter->funcs())
                log_line << function << ", ";
            log_line << "]; ";
        }
        else if (FilterFile* file_filter = dynamic_cast<FilterFile*>(filter))
        {
            log_line << "type: file_name";
            log_line << "files: [ ";
            for (const string& file : file_filter->files())
                log_line << file << ", ";
            log_line << "]; ";
        }
        else if (FilterThread* file_thread = dynamic_cast<FilterThread*>(filter))
        {
            log_line << "type: thread_id";
            log_line << "threads: [ ";
            for (pid_t tid : file_thread->threads())
                log_line << long(tid) << ", ";
            log_line << "]; ";
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
