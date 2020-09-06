import qbs
import qbs.ModUtils

CppApplication {
    name: "list_qtest"
    consoleApplication: true
    destinationDirectory: "./"

    Depends { name: "Qt"; submodules: ["core", "test"] }

    cpp.cxxFlags: [
        "-std=c++14",
    ]

    cpp.systemIncludePaths: ModUtils.concatAll(
        Qt.core.cpp.includePaths
    )

    files: [
        "../list.h",
        "list_qtest.cpp"
    ]
}
