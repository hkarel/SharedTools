/*****************************************************************************
  В модуле определены пути до библиотеки opencv.

*****************************************************************************/

import qbs
import '../LibModule.qbs' as LibModule

LibModule {
    id: opencv
    version: "3.0.x"
    prefix: "/opt/opencv"
    Properties {
        condition: qbs.targetOS.contains("windows")
                   && qbs.toolchain && qbs.toolchain.contains("mingw")
        prefix: "c:/opt/opencv"
        libSuffix: "/x86/mingw/lib"
    }
}
