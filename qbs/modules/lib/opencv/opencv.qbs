/*****************************************************************************
  В модуле определены пути до библиотеки opencv.

*****************************************************************************/

import qbs
import '../LibModule.qbs' as LibModule

LibModule {
    id: opencv
    prefix: "/opt/opencv"
    version: "3.0.x"
    libSuffix: {
        if (qbs.targetOS.contains("windows")
            && qbs.toolchain && qbs.toolchain.contains("mingw"))
        {
            return "/x86/mingw/lib";
        }
        return "/lib";
    }

}
