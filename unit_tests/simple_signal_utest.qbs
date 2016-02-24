import qbs 1.0

CppApplication {
    name: "simple_signal_utest"

    consoleApplication: true
    destinationDirectory: "./"

        cpp.cxxFlags: [
        "-std=c++11",
    ]

    cpp.includePaths: [
        "../",
        "../closure",
    ]

    cpp.dynamicLibraries: [
        "pthread",
    ]

    files: [
        "../simple_signal.h",
        "../closure/closure3.h",
        "simple_signal_utest.cpp",
    ]

}
