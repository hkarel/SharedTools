import qbs

CppApplication {
    name: "shared_ptr_speed"
    consoleApplication: true
    destinationDirectory: "./"

    cpp.cxxFlags: [
        "-Wno-unused-parameter",
    ]
    cpp.cxxLanguageVersion: "c++17"

    files: [
        "../container_ptr.h",
        "shared_ptr_speed.cpp"
    ]
}
