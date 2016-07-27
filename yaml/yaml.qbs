import qbs
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
        "include/yaml-cpp/*.h",
        "include/yaml-cpp/contrib/*.h",
        "include/yaml-cpp/node/*.h",
        "include/yaml-cpp/node/detail/*.h",
        "src/*.cpp",
        "src/*.h",
    ]
    Export {
        Depends { name: "cpp" }
        cpp.systemIncludePaths: product.cpp.systemIncludePaths
    }
} // Product
