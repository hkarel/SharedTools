import qbs
import qbs.File
import GccUtl

Product {
    type: "staticlibrary"

    name: "Yaml"
    targetName: "yaml"

    Depends { name: "cpp" }

    cpp.archiverName: GccUtl.ar(cpp.toolchainPathPrefix)
    cpp.cxxFlags: [
        "-std=c++11",
        "-ggdb3",
        "-Wall",
        "-Wextra",
        "-Wno-unused-parameter",
    ]
    cpp.systemIncludePaths: [
        "include",
    ]
    files: [
        "src/*.h",
        "src/*.cpp",
    ]
    Export {
        Depends { name: "cpp" }
        cpp.systemIncludePaths: product.cpp.systemIncludePaths
    }
} // Product
