import qbs

CppApplication {
    name: "LoggerTest2"
    targetName: "logger_test2"
    condition: true

    type: "application"
    consoleApplication: true
    destinationDirectory: "./"

    cpp.defines: {
        var def = [];
        if (qbs.buildVariant === "release")
            def.push("NDEBUG");
        return def;
    }

    cpp.cxxFlags: [
        "-ggdb3",
        //"-Winline",
        "-Wall",
        "-Wextra",
        "-Wno-unused-parameter",
        "-Wno-variadic-macros",
    ]
    cpp.cxxLanguageVersion: "c++14"

    cpp.includePaths: [
        "../",
    ]

    cpp.dynamicLibraries: [
        "pthread",
    ]

    files: [
        "../list.h",
        "../clife_base.h",
        "../clife_ptr.h",
        "../simple_ptr.h",
        "../safe_singleton.h",
        "../utils.cpp",
        "../utils.h",
        "../logger/logger.cpp",
        "../logger/logger.h",
        "../thread/thread_base.cpp",
        "../thread/thread_base.h",
        "../thread/thread_pool.cpp",
        "../thread/thread_pool.h",
        "../thread/thread_utils.cpp",
        "../thread/thread_utils.h",
        "logger_test2.cpp",
    ]
}
