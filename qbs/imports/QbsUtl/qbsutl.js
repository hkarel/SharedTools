// Создает полный путь до статических библиотек.
function buildFullNamesLibraries(product, path, libraries)
{
    var libs = [];
    var staticLibraryPrefix = product.cpp.staticLibraryPrefix;
    var staticLibrarySuffix = product.cpp.staticLibrarySuffix;

    for (var i in libraries)
        libs.push(path + "/" + staticLibraryPrefix + libraries[i] + staticLibrarySuffix)
    return libs;
}

// Расширяет тип String функцией форматирования.
// Источник: http://www.helpful-stuff.ru/2013/08/method-string-format-javascript.html
// Использование: "Test formattind {0}, {1}".format(123, "string")
String.prototype.format = function () {
    var args = arguments;
    return this.replace(/\{\{|\}\}|\{(\d+)\}/g, function (m, n) {
        if (m === "{{") { return "{"; }
        if (m === "}}") { return "}"; }
        return args[n];
    });
};

// Функция возвращает информацию о версии проекта в виде массива из пяти элементов.
// Массив имеет следующие элементы:
// 0 - версия проекта в формате major.minor.patch
// 1 - major версия
// 2 - minor версия
// 3 - patch (версия исправлений)
// 4 - revision (номер ревизии, берется из GIT)
function getVersions(project)
{
    var filePath = project.sourceDirectory + "/VERSION";
     if (!File.exists(filePath))
         throw new Error(("File '{0}' not found").format(filePath));

     var file = new TextFile(filePath, TextFile.ReadOnly);
     var verStr = file.readLine().trim();

     var regex = /^\d+\.\d+\.\d+$/
     var r = verStr. match(regex);
     if (r === null) {
         var msg =  "Incorrect version format. Must be: 'major.minor.patch'. See file {0}";
         throw new Error(msg.format(filePath));
     }

     var verParts = verStr.split('.').map(function(item){return parseInt(item, 10);});

     var verRevision;
     var process = new Process();
     process.setWorkingDirectory(project.sourceDirectory);
     if (process.exec("git", ["log", "-1", "--pretty=%h"], false) === 0)
         verRevision = process.readLine().trim();

    return [verStr, verParts[0], verParts[1], verParts[2], verRevision];
}
