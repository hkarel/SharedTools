/*****************************************************************************
  В модуле определены пути до gmock библиотеки и флаги компиляции.

*****************************************************************************/

import qbs
import qbs.File

Module {
    id: gmock

    property path toolchainPath: "/usr/src/gmock"
    property bool exists: {return File.exists(toolchainPath);}

    property pathList includePaths: [
        toolchainPath,
        toolchainPath + "/include",
        toolchainPath + "/gtest",
        toolchainPath + "/gtest/include",
    ]
}
