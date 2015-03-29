import qbs 1.0

CppApplication {
    name: "list_utest"

    consoleApplication: true

    //cpp.includePaths: includePaths
    cpp.cxxFlags: [
        "-std=c++11",
        "-Wno-unused-parameter",
    ]

    files: [
        "../_list.h",
        "list_utest.cpp"
    ]
}
