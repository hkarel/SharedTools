import qbs 1.0

CppApplication {
    name: "closure3_utest"

    consoleApplication: true
    destinationDirectory: "./"

    cpp.cxxFlags: [
        "-std=c++14",
        "-Wno-unused-parameter",
    ]

    cpp.includePaths: [
        "../",
        "../closure",
    ]

    cpp.dynamicLibraries: [
        "pthread",
    ]

    files: [
        "../closure/closure3.h",
        "closure3_utest.cpp",
    ]

}
