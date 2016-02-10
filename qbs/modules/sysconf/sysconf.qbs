/*****************************************************************************
  В модуле определены системно зависимые параметры необходимые для сборки
  проекта на различных Linux/Unix системах.
  Значения для данного модуля задаются в qbs профиле.
*****************************************************************************/

import qbs

Module {
    id: sysconf
    condition: true

    property pathList includePaths
    property pathList libraryPaths

    PropertyOptions {
        name: "libraryPaths"
        description: "Определяет дополнительные пути расположения библиотек"
    }

    PropertyOptions {
        name: "includePaths"
        description: "Определяет дополнительные пути расположения header-файлов"
    }
}
