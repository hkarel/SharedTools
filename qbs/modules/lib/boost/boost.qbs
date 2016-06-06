/*****************************************************************************
  В модуле определены пути до boost библиотеки.

*****************************************************************************/

import qbs
import '../LibModule.qbs' as LibModule

LibModule {
    id: boost
    version: "1.55.x"
    prefix: "/opt/boost"
    Properties {
        condition: qbs.targetOS.contains("windows")
                   && qbs.toolchain && qbs.toolchain.contains("mingw")
        prefix: "c:/opt/boost"
        includeSuffix: ""
        libSuffix: ""
    }
}
