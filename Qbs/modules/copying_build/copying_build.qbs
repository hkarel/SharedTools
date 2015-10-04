/*****************************************************************************
  Вспомогательный модуль, используется для копирования собранных исполняемых
  файлов и динамических библиотек в указанную директорию

*****************************************************************************/

import qbs
import qbs.File
import qbs.FileInfo
import qbs.ModUtils

Module {
    id: copying_build
    additionalProductTypes: "copied_build"

    property path targetDirectory
    property string type

    PropertyOptions {
        name: "targetDirectory"
        description: "Директория в которую буду копироваться собранные product's"
    }

    PropertyOptions {
        name: "type"
        description: "Тип приложения"
        allowedValues: ['application', 'dynamiclibrary']
    }

    Rule {
        // Не удалось решить задачу определения inputs одновременно для двух
        // типов собираемых модулей. Поэтому тип собираемого модуля задается
        // явно в целевом сценарии через переменную type.
        //inputs: ["application", "dynamiclibrary"]

        inputs: {
            //["application"]
            return type;
        }
        outputFileTags: ["copied_build"]
        outputArtifacts: {
            var destinationDir = ModUtils.moduleProperty(product, "targetDirectory");
            if (!destinationDir)
                destinationDir = project.sourceDirectory;

            return [{
                filePath: destinationDir + '/' + input.fileName,
                fileTags: ["copied_build"]
            }];
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "copying " + FileInfo.fileName(input.fileName);
            cmd.highlight = "codegen";
            cmd.sourceCode = function() { File.copy(input.filePath, output.filePath); };
            return cmd;
        }
    }

} // Module
