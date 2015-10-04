/*****************************************************************************
  В модуле определены методы для получения версии проекта, а также методы
  получения git-ревизии проекта.

*****************************************************************************/

import qbs
import qbs.File
import qbs.TextFile
import qbs.Process
import QbsUtl

Module {
    id: project_version
    condition: true

    // Так можно задать версию из командной строки:
    //   qbs build -v debug project_version.fullBuildVersion:2.0.17.0
    property string fullBuildVersion: {
        var filePath = project.sourceDirectory + "/VERSION";
        if (!File.exists(filePath))
            throw new Error(("File '{0}' not found").format(filePath));

        var file = new TextFile(filePath, TextFile.ReadOnly);
        var vers = file.readLine().trim();

        var regex = /^\d+\.\d+\.\d+\.\d+$/
        var r = vers. match(regex);
        if (r == null) {
            var msg =  "Incorrect version format. Must be: 'N.N.N.N'. See file {0}";
            throw new Error(msg.format(filePath));
        }
        return vers;
    }

    property string buildVersion: {
        var regex = /^(\d+\.\d+)\.(\d+\.\d+)$/
        var r = regex.exec(fullBuildVersion);
        return r[1];
    }

    property string buildName: {
        var regex = /^(\d+\.\d+)\.(\d+\.\d+)$/
        var r = regex.exec(fullBuildVersion);
        return r[2];
    }

    property string buildRevision: {
        var process = new Process();
        process.setWorkingDirectory(project.sourceDirectory);
        if (process.exec("git", ["log", "-1", "--pretty=%h"], false) === 0)
            return process.readLine().trim();
    }

    property var cppDefines: {
        var def = [];

        if (buildVersion && buildVersion.length)
            def.push("VERSION=\"" + buildVersion + "\"");

        if (buildName && buildName.length)
            def.push("BUILDNAME=\"" + buildName + "\"");

        if (buildRevision.length)
            def.push("BUILDREV=\"" + buildRevision + "\"");

        return def;
    }

     PropertyOptions {
         name: "fullBuildVersion"
         description: "Определяет полное обозначение версии проекта"
     }

     PropertyOptions {
         name: "buildVersion"
         description: "Определяет базовую версию продукта"
     }

     PropertyOptions {
         name: "buildName"
         description: "Определяет версию сборки продукта"
     }

     PropertyOptions {
         name: "buildRevision"
         description: "Номер ревизии, берется из GIT"
     }

} // Module
