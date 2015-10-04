/*****************************************************************************
  В модуле определены пути до python библиотеки.

*****************************************************************************/

import qbs
import qbs.File
import QbsUtl

Module {
    id: python

    property string prefix: "/opt/python"
    property string version: "2.7.x"

    property pathList includePaths: {
        return [].concat(
            prefix + "/" + version + "/include",
            prefix + "/" + version + "/include/python2.7"
        );
    }

    property path libraryPath:  prefix + "/" + version + "/lib"
    property path libraryPathA: prefix + "/" + version + "/lib/python2.7/config"

    PropertyOptions {
        name: "includePaths"
        description: "Пути расположения header-файлов"
    }

    PropertyOptions {
        name: "libraryPath"
        description: "Путь расположения динамической библиотеки"
    }

    PropertyOptions {
        name: "libraryPathA"
        description: "Путь расположения статической библиотеки"
    }

    property var probe: {
        return function() {
            var msg = "Python directory {0} not found. Possibly incorrect assigned version for pythom module.";
            for (var i in includePaths)
                if (!File.exists(includePaths[i]))
                    throw msg.format(includePaths[i]);

            if (!File.exists(libraryPath))
                throw msg.format(libraryPath);

            if (!File.exists(libraryPathA))
                throw msg.format(libraryPathA);
        };
    }

} // Module
