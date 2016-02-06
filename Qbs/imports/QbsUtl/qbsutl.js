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
