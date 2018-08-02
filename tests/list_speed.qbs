import qbs 1.0

CppApplication {
    name: "list_speed"
    consoleApplication: true
    destinationDirectory: "./"

    cpp.cxxFlags: [
        "-std=c++11",
        "-ggdb3",
        "-Wno-unused-parameter",
    ]

    files: [
        "../list.h",
        "list_speed.cpp"
    ]
}
