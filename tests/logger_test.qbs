import qbs

CppApplication {
    name: "logger_test"
    consoleApplication: true
    destinationDirectory: "./"

    cpp.cxxFlags: [
        "-std=c++11",
    ]

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
        "../thread/thread_info.cpp",
        "../thread/thread_info.h",
        "../thread/thread_pool.cpp",
        "../thread/thread_pool.h",
        "logger_test.cpp",
    ]
}
