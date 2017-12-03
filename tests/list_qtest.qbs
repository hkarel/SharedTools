import qbs

CppApplication {
    name: "list_qtest"
    consoleApplication: true
    destinationDirectory: "./"

    Depends { name: "Qt"; submodules: ["core", "test"] }

    cpp.cxxFlags: [
        "-std=c++11",
    ]

    files: [
        "../_list.h",
        "list_qtest.cpp"
    ]
}
