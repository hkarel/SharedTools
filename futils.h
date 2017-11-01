/*****************************************************************************
  The MIT License

  Copyright © 2010 Pavel Karelin (hkarel), <hkarel@yandex.ru>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  ---

  Модуль содержит функции общего назначения.

*****************************************************************************/

#ifndef futils_H
#define futils_H

#include <QtCore>
#include "defmac.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#if defined(Q_OS_WIN) && defined(USE_WIN_COM)
#include <comutil.h>
#endif


namespace futl { 


/** @brief Прверяет наличие слеша в конце строки */
template<typename StringT>
inline bool checkLastSlash(const /*QString*/ StringT &s) 
    {return (s.length() && ((s[s.length()-1] == QChar('\\')) || (s[s.length()-1] == QChar('/'))));}

/** @brief Прверяет наличие слеша в конце строки, если слеша нет - он будет добавлен */
template<typename StringT>
inline /*QString*/ StringT& addLastSlash(/*QString*/ StringT& s, QChar sep = '/' /*QDir::separator()*/) {
    return 
        (s.length() && !((s[s.length()-1] == QChar('\\')) || (s[s.length()-1] == QChar('/')))) 
        //? s += QChar('\\') : s; 
        ? s += sep /*QDir::separator()*/ : s; 
}


#ifdef QT_VERSION

#ifdef Q_OS_WIN
/** @brief Возвращает строковое описание ошибки для функции WinAPI GetLastError() */
//QString getLastErrorString(DWORD errorCode);
//inline QString getLastErrorString() {return getLastErrorString(GetLastError());}
#endif //Q_OS_WIN

/** @brief Проверяет содержится ли строка str в списке list. Поиск может идти с учетом 
 или без учета регистра символов. В случае успеха возвращается индекс найденного
 элемента в списке list, в случае неудачи возвращается -1.
*/
int qStringListContains(const QStringList &list, const QString &str, Qt::CaseSensitivity cs);
inline int qStringListContainsIC(const QStringList &list, const QString &str)
    {return qStringListContains(list, str, Qt::CaseInsensitive);}

#if defined(Q_OS_WIN) && defined(USE_WIN_COM)
/** @brief Функции конвертации строк  
    Для корректной работы директивы USE_WIN_COM - функция 
    должна быть определена в хедере.
*/
inline bstr_t toBstr(const QString &s) {
    wchar_t *ws = ::SysAllocStringByteLen(NULL, s.length() * sizeof(wchar_t));
    s.toWCharArray(ws);
    bstr_t bs;
    bs.Attach(ws);
    return bs;
}
inline QString toQString(const bstr_t &s) 
    {return QString::fromWCharArray(s);} 
#endif //Q_OS_WIN

// inline QString toQString(float n, char format = 'g', int precision = 6)
//     {QString s; return s.setNum(n, format, precision);}    
// inline QString toQString(double n, char format = 'g', int precision = 6)
//     {QString s; return s.setNum(n, format, precision);}    
// template<typename NumberT> inline QString toQString(NumberT n, int base = 10)
//     {QString s; return s.setNum(n, base);}
// //---------------------------------------------------------------------------

/** @brief Замена слешей */
inline QString& slashReplace(QString &s, QChar source = '\\', QChar dest = '/')
    //{for (int i = 0; i < s.length(); ++i) if (s[i] == source) s[i] = dest; return s;}
    {return s.replace(source, dest);}

inline QString& slashReplace(QString &s, char source, char dest)
    {return s.replace(QChar(source), QChar(dest));}

inline QByteArray& slashReplace(QByteArray &s, char source = '\\', char dest = '/')
    //{for (int i = 0; i < s.length(); ++i) if (s[i] == source) s[i] = dest; return s;}
    {return s.replace(source, dest);}


template<typename StringT> inline StringT& toNativeSlash(StringT& s) {
#ifdef Q_OS_WIN    
    return slashReplace(s, '/', '\\');
#else
    return slashReplace(s, '\\', '/');
#endif
}


/** @brief Заменяет слеши на нативный сепаратор */
//inline toNativeSeparator

/** @brief Функции сравнения строк с учетом регистра 
*/
inline bool equalStr(const QString &s1, const QString &s2) 
    {return (s1.compare(s2, Qt::CaseSensitive) == 0);}

inline bool equalStr(const QByteArray &s1, const QByteArray &s2) 
    {return (qstrcmp(s1.constData(), s2.constData()) == 0);}

inline bool equalStr(const char *s1, const char *s2) 
    {return (qstrcmp(s1, s2) == 0); }

inline bool equalStr(const QString &s1, const QByteArray &s2) 
    {return (qstrcmp(s1.toUtf8().constData(), s2.constData()) == 0);}

inline bool equalStr(const QByteArray &s1, const QString &s2) 
    {return equalStr(s2, s1);}

inline bool equalStr(const QByteArray &s1, const char *s2) 
    {return (qstrcmp(s1.constData(), s2) == 0);}

inline bool equalStr(const char *s1, const QByteArray &s2) 
    {return equalStr(s2, s1);}

inline bool equalStr(const QString &s1, const char *s2) 
    {return (qstrcmp(s1.toUtf8().constData(), s2) == 0);}

inline bool equalStr(const char *s1, const QString &s2) 
    {return equalStr(s2, s1);}


/** @brief Функции сравнения строк с без учета регистра  
*/
inline bool equalStrIC(const QString &s1, const QString &s2) 
    {return (s1.compare(s2, Qt::CaseInsensitive) == 0);}

inline bool equalStrIC(const QByteArray &s1, const QByteArray &s2) 
    {return (qstricmp(s1.constData(), s2.constData()) == 0);}

inline bool equalStrIC(const char *s1, const char *s2) 
    {return (qstricmp(s1, s2) == 0);}

inline bool equalStrIC(const QString &s1, const QByteArray &s2) 
    {return (qstricmp(s1.toUtf8().constData(), s2.constData()) == 0);}

inline bool equalStrIC(const QByteArray &s1, const QString &s2) 
    {return equalStrIC(s2, s1); }

inline bool equalStrIC(const QByteArray &s1, const char *s2) 
    {return (qstricmp(s1.constData(), s2) == 0);}

inline bool equalStrIC(const char *s1, const QByteArray &s2) 
    {return equalStrIC(s2, s1);}

inline bool equalStrIC(const QString &s1, const char *s2) 
    {return (qstricmp(s1.toUtf8().constData(), s2) == 0);}

inline bool equalStrIC(const char *s1, const QString &s2) 
    {return equalStrIC(s2, s1);}



/**
 @brief Функция проверяет сожержится ли строка s в строковом списке list. 
 Элементы в строковом списке list должны быть разделены следующими 
 разделителями: ' '  ','  ';' 
*/
bool strInList(const QString &s, const QString &list, Qt::CaseSensitivity cs = Qt::CaseSensitive);

bool strInList(const char *s, const char *list);
inline bool strInList(const QByteArray &s, const QByteArray &list)
    {return strInList(s.constData(), list.constData());}
inline bool strInList(const QByteArray &s, const char *list)
    {return strInList(s.constData(), list);}
inline bool strInList(const char *s, const QByteArray &list)
    {return strInList(s, list.constData());}

/** @brief Аналог ф-ции strInList, проводит сравнение без учета регистра */
inline bool strInListIC(const QString &s, const QString &list)
    {return strInList(s, list, Qt::CaseInsensitive);}
//---------------------------------------------------------------------------

bool strStartsWith(const QString &s, const QString &list, Qt::CaseSensitivity cs = Qt::CaseSensitive);

#ifdef Q_OS_WIN
/** @brief Преобразует короткий путь (формат 8.3) в полный путь */
bool getLongPathName(const QString &shortPath, QString &longPath);
/// @brief Преобразует полный путь в короткий путь (формат 8.3)
bool getShortPathName(const QString &longPath, QString &shortPath);
#endif //Q_OS_WIN

/** @brief Сравнивает пути без учета регистра и разницы слешей.
  Таким образом следующие пути будут эквивалентны: "c:\Temp" == "c:/temp/"
*/
bool equalPath(QString path1, QString path2);
               

#ifdef Q_OS_WIN
/** @brief Обертка для API-функции ExpandEnvironmentStrings */
QString expandEnvironmentStrings(const QString& s);

/** @brief Возвращает наименование системной директории */
QString systemRoot();

/** @brief Возвращает путь до временной директории текущего пользователя */
QString tempDir();

/** @brief Возвращает наименование системного диска */
QString systemDisk();


/** @brief Создает GUID */
//inline bool createGUID(GUID &guid) {return (::CoCreateGuid(&guid) == S_OK);}
//GUID createGUID(bool *ok = NULL);

/** @brief Преобразует GUID в String */
//bool guidToString(const GUID &, QString &);
//QString guidToString(const GUID &, bool *ok = NULL);

/** @brief Создает и возвращает GUID в строковом представлении */
//bool createStringGUID(QString &s);
//QString createStringGUID(bool *ok = NULL);

/** @brief Возвращает путь расположения модуля по его HINSTANCE */
QString moduleFileName(HINSTANCE instance);

#endif //Q_OS_WIN

/** @brief Возвращает расширение файла без символа '.', возвращается только 
    первичное расширение, составные расширения (например .tar.gz) не возвращаются.
*/
QString fileExtension(const QString& fileName);

/** @brief Заменяет расширение файла. 
    Первым символомом в newExt должна быть точка, например так: ".ini" 
*/
QString replaceFileExtension(const QString& fileName, const QString& newExt);

/** @brief Удаляет файл. В случае удачного завершения возвращает TRUE. */
bool deleteFile(QString fileName);

/** @brief Удаляет директорию при условии, что она пустая. 
    В случае удачного завершения возвращает TRUE.
*/
bool deleteDirectory(QString dirName);

/** @brief Проверяет существование файла или директории. 
    Для использования функции необходимо подключить shlwapi.lib
*/
bool pathFileExists(QString path);

/** @brief Проверяет существование файла или директории. */
inline bool fileExists(const QString& fileName) {return pathFileExists(fileName);}
inline bool directoryExists(const QString& dirName) {return pathFileExists(dirName);}

/** @brief Проверяет является ли указанный путь директорией */
bool isDirectory(QString path);


// Возвращает директорию размещения испольняемого файла, причем если
// приложение запущено из отладочных директорй (debug/release), то эти
// директории будут исключены из пути.
// Если параметр fileName определен, то будет возвращен путь до файла,
// как будто этот файл расположен в директории приложения.
QString applPath(const QString& fileName = "");



// Возвращает родителя заданного типа
template<typename ParentT> ParentT* getParentWidget(QWidget* wgt) {
    //QWidget* parent = wgt;
    while (true) {
        if (wgt == 0) break;
        if (ParentT* w = qobject_cast<ParentT*>(wgt))
            return w;
        wgt = wgt->parentWidget();
    }
    return 0;
}

template<typename ParentT> ParentT* getParentObject(QObject* obj) {
    while (true) {
        if (obj == 0) break;
        if (ParentT* o = qobject_cast<ParentT*>(obj))
            return o;
        obj = obj->parent();
    }
    return 0;
}




#endif //QT_VERSION



//QDataStream& operator << (QDataStream&, const GUID&);
//QDataStream& operator >> (QDataStream&, GUID&);





}; /*namespace futl*/

using namespace futl;

#endif //#ifndef futils_H
