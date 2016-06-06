/*****************************************************************************
 Базовый модуль, используется для описания и подключения сторонних библиотек.

*****************************************************************************/

import qbs
import qbs.File
import QbsUtl

Module {
    property string prefix
    property string version

    // Признак использования системной библиотеки
    property bool useSystem: false

    property string includeSuffix: "/include"
    property string libSuffix: "/lib"

    property path includePath: !useSystem ? prefix + "/" + version + includeSuffix : undefined
    property path libraryPath: !useSystem ? prefix + "/" + version + libSuffix : undefined

    property var probe: {
        return function() {
            if (useSystem)
                return;
            var msg = "Module {0}: directory '{1}' not found. Possibly incorrect assigned version ({2}).";
            if (!File.exists(includePath))
                throw msg.format(name, includePath, version);

            if (!File.exists(libraryPath))
                throw msg.format(name, libraryPath, version);
        };
    }
}
