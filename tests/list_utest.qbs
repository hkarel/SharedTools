import qbs 1.0

CppApplication {
    name: "list_utest"
    consoleApplication: true
    destinationDirectory: "./"

    cpp.cxxFlags: [
        "-std=c++11",
        "-ggdb3",
        "-Wno-unused-parameter",
    ]

    //cpp.linkerFlags: [
    //    "-static-libstdc++",
    //    "-static-libgcc",
    //]

    files: [
        "../_list.h",
        "list_utest.cpp"
    ]
}
