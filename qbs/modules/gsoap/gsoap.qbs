import qbs
import qbs.File
import qbs.TextFile
import qbs.ModUtils


Module {
    id: gsoap
    condition: true

    Depends { name: "cpp" }

    property string namespace: undefined
    property string modulePath: ""
    property string importPath: ""
    property string soapcpp2: ""
    property bool   debug: false

    property string generatedFilesDir: {
        return product.buildDirectory + "/." + (namespace === undefined ? "soap" : namespace);
    }

    property string includePaths: {
        return generatedFilesDir;
    }

    PropertyOptions {
        name: "namespace"
        description: "Определяет имя пространства имен для функций soap-движка."
    }
    PropertyOptions {
        name: "modulePath"
        description: "Директория расположения файлов stdsoap2.h, stdsoap2.cpp"
    }
    PropertyOptions {
        name: "importPath"
        description: "Директория расположения файлов импорта gsoap"
    }
    PropertyOptions {
        name: "soapcpp2"
        description: "Утилита soapcpp2"
    }
    PropertyOptions {
        name: "debug"
        description: "Сборка в режиме отладки"
    }
    PropertyOptions {
        name: "generatedFilesDir"
        description: "Директория размещения сгенерированных файлов, так же \
                      в эту директорию копируются файлы stdsoap2.h, stdsoap2.cpp"
    }

    cpp.defines: {
        var def = [

            // --- Для сборки LBcore с gsoap 2.7.12 этот флаг использовать не нужно ---
            //"WITH_NONAMESPACES", // С этим флагом нужно явно определять soap-namespace
            //                     // в рабочем коде.

            "WITH_COOKIES",
            "WITH_GZIP",
            "WITH_OPENSSL",

            // "WITH_NOGLOBAL",
            // Важно: Необходимо пропатчить файл stdsoap2.h, нужно добавить в него
            // следующие директивы:
            //    #ifdef WITH_NOGLOBAL
            //    #undef WITH_NOGLOBAL
            //    #endif
            //
            // Сделать это нужно перед первым упоминанием "namespace soap2xxx"
            //    #ifdef __cplusplus
            //    //extern "C" {
            //    namespace soap2xxx {
            //    #endif
            //

            // Необходимо пропатчить файл stdsoap2.cpp, в нем нужно заменить строку 'extern "C" {'
            // на 'namespace soap2xxx {'
            //    #ifdef __cplusplus
            //    SOAP_SOURCE_STAMP("@(#) stdsoap2.cpp ver 2.7.12 2008-10-01 00:00:00 GMT")
            //    //extern "C" {
            //    namespace soap2xxx {
            //    #else
            //    SOAP_SOURCE_STAMP("@(#) stdsoap2.c ver 2.7.12 2008-10-01 00:00:00 GMT")
            //    #endif

            // Так же нужно конструкторы структуры soap внести в пространство namespace soap2xxx.
            // Этот код должен быть расположен в самом конце файла перед закрывающей скобкой
            // пространства имен '} // namespace soap2712', а код идущий ниже шапки
            // 'C++ soap struct methods' нужно либо удалить, либо закомментировать.
            //    #ifdef __cplusplus
            //    soap::soap()
            //    { soap_init(this);
            //    }
            //    soap::soap(soap_mode m)
            //    { soap_init1(this, m);
            //    }
            //    soap::soap(soap_mode im, soap_mode om)
            //    { soap_init2(this, im, om);
            //    }
            //    soap::soap(const struct soap& soap)
            //    { soap_copy_context(this, &soap);
            //    }
            //    soap::~soap()
            //    { soap_destroy(this);
            //      soap_end(this);
            //      soap_done(this);
            //    }

            //    } // namespace soap2712
            //    #endif

            /******************************************************************************\
             *
             *  C++ soap struct methods
             *
            \******************************************************************************/

            //#ifdef __cplusplus
            //soap::soap()
            //{ soap_init(this);
            //}
            //#endif

            // ...
        ];

        if (debug === true)
            def.push("SOAP_DEBUG");

        return def;
    }

    cpp.includePaths: {
        return this.includePaths;
    }

    validate: {
        var validator = new ModUtils.PropertyValidator("gsoap");
        //validator.setRequiredProperty("namespace", namespace);
        validator.setRequiredProperty("modulePath", modulePath);
        validator.setRequiredProperty("importPath", importPath);
        validator.setRequiredProperty("soapcpp2", soapcpp2);
        validator.validate();

        if (!File.exists(modulePath + "/stdsoap2.h"))
            throw "Base soap-header file not found: " + modulePath + "/stdsoap2.h";

        if (!File.exists(modulePath + "/stdsoap2.cpp"))
            throw "Base soap-source file not found: " + modulePath + "/stdsoap2.cpp";
    }

     //FileTagger {
     //    patterns: ["*.wsdl"]
     //    fileTags: ["wsdl"]
     //}

    FileTagger {
        patterns: "*.soap.h"
        fileTags: ["soap"]
    }

    Rule {
        inputs: ["soap"]
        outputFileTags: ["cpp", "hpp"]
        outputArtifacts: {
            var generatedFilesDir = ModUtils.moduleProperty(product, "generatedFilesDir");
            var namespace = ModUtils.moduleProperty(product, "namespace");
            if (namespace === undefined)
                namespace = "soap";

            var cxxFlags = [
                //"-Wno-unused-function",
                //"-Wno-unused-but-set-variable",
                //"-Wno-unused-parameter",
                //"-Wno-sequence-point",
                //"-Wno-literal-suffix",
                //"-Wno-sizeof-pointer-memaccess",
                "-w",
            ]

            var result = [
                {
                    filePath: generatedFilesDir + "/stdsoap2.h",
                    fileTags: ["hpp"],
                    cpp: {
                        cxxFlags: cxxFlags
                    }
                },{
                    filePath: generatedFilesDir + "/stdsoap2.cpp",
                    fileTags: ["cpp"],
                    cpp: {
                        cxxFlags: cxxFlags
                    }
                },{
                    filePath: generatedFilesDir + "/" + namespace + "C.cpp",
                    fileTags: ["cpp"],
                    cpp: {
                        cxxFlags: cxxFlags
                    }
//                },{
//                    filePath: generatedFilesDir + "/" + namespace + "Client.cpp",
//                    fileTags: ["cpp"],
//                    cpp: {
//                        cxxFlags: cxxFlags
//                    }
                },{
                    filePath: generatedFilesDir + "/" + namespace + "Server.cpp",
                    fileTags: ["cpp"],
                    cpp: {
                        cxxFlags: cxxFlags
                    }
                },{
                    filePath: generatedFilesDir + "/" + namespace + "H.h",
                    fileTags: ["hpp"],
                    cpp: {
                        cxxFlags: cxxFlags
                    }
                },{
                    filePath: generatedFilesDir + "/" + namespace + "Stub.h",
                    fileTags: ["hpp"],
                    cpp: {
                        cxxFlags: cxxFlags
                    }
                }
            ];

            return result;
        }

        prepare: {
            var commands = [];

            var cmd = new JavaScriptCommand();
            cmd.silent = true;
            cmd.sourceCode = function() {
                var generatedFilesDir = ModUtils.moduleProperty(product, "generatedFilesDir");
                var modulePath = ModUtils.moduleProperty(product, "modulePath");

                if (!File.exists(generatedFilesDir + "/stdsoap2.h"))
                    File.copy(modulePath + "/stdsoap2.h", generatedFilesDir + "/stdsoap2.h");

                if (!File.exists(generatedFilesDir + "/stdsoap2.cpp"))
                    File.copy(modulePath + "/stdsoap2.cpp", generatedFilesDir + "/stdsoap2.cpp");
            }
            commands.push(cmd);

            var namespace = ModUtils.moduleProperty(product, "namespace");
            var importPath = ModUtils.moduleProperty(product, "importPath");
            var generatedFilesDir = ModUtils.moduleProperty(product, "generatedFilesDir");

            //'/opt/gsoap/2.7.12/bin/soapcpp2 -x -L -S  -I xmlapi/import -d xmlapi xmlapi/api3.h'
            //var args = ["-C", "-w", "-x", "-n", "-L"];
            var args = ["-x", "-L"];

            if (namespace !== undefined) {
                args.push("-q");
                args.push(namespace);
            }

            args.push("-I");
            args.push(importPath);

            args.push("-d");
            args.push(generatedFilesDir);

            //args.push(product.sourceDirectory + "/" + "hybrid-api.soap.h")
            args.push(input.filePath);

//             print("=== 111 ===");
//             print(inputs);
//
//             print("=== 222 ===");
//             for (var i in input) {
//                 print("222");
//                 print(i);
//             }
//
//             print("=== 333 ===");
//             for (var i in product) {
//                 print("333");
//                 print(i);
//                 print(product[i]);
//                 print(typeof product[i]);
//                 if (typeof product[i] == 'object') {
//                     var obj = product[i];
//                     for (var j in obj) {
//                         print(j);
//                         print(obj[j]);
//                         if (typeof obj[j] == 'object') {
//                             var obj2 = obj[j];
//                             for (var k in obj2) {
//                                 print(k);
//                                 print(obj2[k]);
//                             }
//                         }
//                     }
//
//                 }
//             }

            var soapcpp2 = ModUtils.moduleProperty(product, "soapcpp2");
            cmd = new Command(soapcpp2, args);

            cmd.description = "soap generation: ";
            cmd.highlight = "codegen";
            commands.push(cmd);

            if (namespace !== undefined) {
                cmd = new JavaScriptCommand();
                cmd.silent = true;
                // ??? cmd.sourceFileName = outputs["c"][0].filePath;
                // ??? cmd.headerFileName = outputs["hpp"] ? outputs["hpp"][0].filePath : "";
                cmd.sourceCode = function() {
                    var namespace = ModUtils.moduleProperty(product, "namespace");
                    var generatedFilesDir = ModUtils.moduleProperty(product, "generatedFilesDir");

                    var file = new TextFile(generatedFilesDir + "/" + namespace + ".nsmap", TextFile.ReadOnly);
                    var text = file.readAll();
                    file.close();

                    text = text.replace(/struct\s+Namespace/, "struct " + namespace + "::Namespace");

                    var file = new TextFile(generatedFilesDir + "/" + namespace + ".nsmap.h", TextFile.WriteOnly);
                    file.write(text);
                    file.close();
                };
                commands.push(cmd);
            }

            return commands;
        }
    }

} // Module
