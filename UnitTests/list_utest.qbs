import qbs 1.0

CppApplication {
    name: "list_utest"

    consoleApplication: true

    //cpp.includePaths: includePaths
    cpp.cxxFlags: [
        "-std=c++11",
    ]

//     cpp.includePaths: [
//         //"../CommonFiles",
//         //"../CommonFiles/Closure",
//         "/usr/include/qt4",
//         "/usr/include/qt4/QtCore",
//     ]


    files: [
        "../_list.h",
        "list_utest.cpp"
    ]

    property bool testData: {
//         print("=== test ===");
//         for (var i in cpp.includePaths) {
//             print(cpp.includePaths[i]);
//         }
//         for (var i in cpp.systemIncludePaths) {
//             print(cpp.systemIncludePaths[i]);
//         }


        // Пример для отображения defQtCoreLib:
        //  qbs -f container_ptr_utest.qbs  profile:gcc container_ptr_utest.defQtCoreLib:true
        //print(defQtCoreLib);
        //print(qbs.profile);

        //return true;
    }
}
