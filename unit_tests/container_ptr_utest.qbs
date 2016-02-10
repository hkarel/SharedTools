import qbs 1.0

CppApplication {
    name: "container_ptr_utest"

    consoleApplication: true
    destinationDirectory: "./"

    Depends { name: "cpp" }

    cpp.cxxFlags: [
        "-std=c++11",
    ]

    cpp.defines: [
        "CONTAINER_PTR_DEBUG",
    ]

    files: [
        "../container_ptr.h",
        "container_ptr_utest.cpp"
    ]

    property bool testData: {
        print("=== test ===");
        for (var i in cpp.includePaths) {
            print(cpp.includePaths[i]);
        }
        for (var i in cpp.systemIncludePaths) {
            print(cpp.systemIncludePaths[i]);
        }
    }
}
