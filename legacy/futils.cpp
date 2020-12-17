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
*****************************************************************************/

//#include <QtTest>
#include "futils.h"

#ifdef Q_OS_WIN
#include "shlwapi.h"
#endif


namespace futl { 


#ifdef QT_VERSION

#ifdef Q_OS_WIN
//QString getLastErrorString(DWORD errorCode)
//{
//    //LPVOID lpMsgBuf;
//    LPTSTR lpMsgBuf[1024] = {0};
//    //LPVOID lpDisplayBuf;
//    //DWORD dw = GetLastError();

//    FormatMessage(
//        /*FORMAT_MESSAGE_ALLOCATE_BUFFER |*/ FORMAT_MESSAGE_FROM_SYSTEM,
//        NULL,
//        errorCode,
//        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//        (LPTSTR) &lpMsgBuf,
//        1024,
//        NULL );

//    QString s;
//    //LPTSTR lpS = (LPTSTR) lpMsgBuf;
//    #ifdef  UNICODE
//    s = QString::fromWCharArray((LPTSTR) lpMsgBuf);
//    #else
//    s =  QString::fromAscii((LPTSTR) &lpMsgBuf);
//    #endif
//    //((LPTSTR) lpMsgBuf);
//    //lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
//    //    (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR));
//    //StringCchPrintf((LPTSTR)lpDisplayBuf,
//    //    LocalSize(lpDisplayBuf),
//    //    TEXT("%s failed with error %d: %s"),
//    //    lpszFunction, dw, lpMsgBuf);
//    //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

//    //LocalFree(lpMsgBuf);
//    //LocalFree(lpDisplayBuf);
//    //ExitProcess(dw);

//    return s;
//}
#endif

int qStringListContains(const QStringList &list, const QString &str, Qt::CaseSensitivity cs)
{
    QStringMatcher matcher(str, cs);
    for (int i = 0; i < list.size(); ++i) {
        const QString& s = list.at(i);
        if ((s.length() == str.length()) && (matcher.indexIn(s) == 0))
            //return QBool(true);
            return i;
    }
    return -1;
}

// #if defined(Q_OS_WIN) && defined(USE_WIN_COM)
// bstr_t toBstr(const QString &s)
// {
//     wchar_t *ws = ::SysAllocStringByteLen(NULL, s.length() * sizeof(wchar_t));
//     s.toWCharArray(ws);
//     bstr_t bs;
//     bs.Attach(ws);
//     return bs;
// }
// #endif

bool strInList(const QString &s, const QString &list, Qt::CaseSensitivity cs)
{
	int ind = 0;
    if (s.length())
        while ((ind = list.indexOf(s, ind, cs)) != -1)
        {
            bool success = true;
            if (list.length() > ind + s.length())
            {
                QChar ch = list[ind + s.length()]; 
                if (! ((ch == ' ') || (ch == ',') || (ch == ';')))
                    success = false;
            }
            if (success && (ind > 0))
            {
                QChar ch = list[ind - 1];
                if (! ((ch == ' ') || (ch == ',') || (ch == ';')))
                    success = false;
            }
            if (success) return true;
            ++ind;
        }
    return false;
}

bool strInList(const char *s, const char *list)
{
    int s_len = qstrlen(s);
    int list_len = qstrlen(list);
    if (s_len && list_len)
    {
    	int ind = 0;
        const QByteArray s_ = QByteArray::fromRawData(s, s_len); 
        const QByteArray list_ = QByteArray::fromRawData(list, list_len); 
        while ((ind = list_.indexOf(s_, ind)) != -1)
        {
            bool success = true;
            if (list_.length() > ind + s_.length())
            {
                char ch = list_[ind + s_.length()]; 
                if (! ((ch == ' ') || (ch == ',') || (ch == ';')))
                    success = false;
            }
            if (success && (ind > 0))
            {
                char ch = list_[ind - 1];
                if (! ((ch == ' ') || (ch == ',') || (ch == ';')))
                    success = false;
            }
            if (success) return true;
            ++ind;
        }
    }
    return false;
}

bool strStartsWith(const QString &s, const QString &list, Qt::CaseSensitivity cs)
{
    QString sbuff;
    QList<QString> list_;

    for (int i = 0; i < list.length(); ++i) {
        QChar ch = list.at(i);
        if ((ch == ' ') || (ch == ',') || (ch == ';')) {
            if (sbuff.length()) {
                list_ << sbuff;
                sbuff.clear();
            }
        }
        else
            sbuff += ch;
    }
    if (sbuff.length()) 
        list_ << sbuff;

    for (int i = 0; i < list_.count(); ++i) 
        if (s.startsWith(list_.at(i), cs))
            return true;
    
    return false;
}

#ifdef Q_OS_WIN
bool getLongPathName(const QString &shortPath, QString &longPath)
{
    //longPath = "";
    if (shortPath.length() == 0)
        return false;

    if (shortPath.contains(QChar('~'))) 
    {
        const DWORD buf_size = 2048;
        wchar_t path_buf[buf_size];
        memset(path_buf, 0, sizeof(path_buf));
        DWORD res = GetLongPathNameW((LPCWSTR)shortPath.utf16(), path_buf, sizeof(path_buf));
        if ((res > buf_size) || (res == 0))
            return false;
        longPath = QString::fromWCharArray(path_buf);
    }
    else 
        longPath = shortPath;
    
    return true;	
}

bool getShortPathName(const QString &longPath, QString &shortPath)
{
    //shortPath = "";
    if (longPath.length() == 0)
        return false;

    if (!longPath.contains(QChar('~')))
    {
        const DWORD buf_size = 2048;
        wchar_t path_buf[buf_size];
        memset(path_buf, 0, sizeof(path_buf));
        DWORD res = GetShortPathNameW((LPCWSTR)longPath.utf16(), path_buf, sizeof(path_buf));
        if ((res == ERROR_INVALID_PARAMETER) || (res == 0))
            return false;
        shortPath = QString::fromWCharArray(path_buf);
    }
    else
        shortPath = longPath;
    
    return true;	
}
#endif //Q_OS_WIN

bool equalPath(QString path1, QString path2)
{
    if ((path1.length() == 0) || (path2.length() == 0))
        return false;

    addLastSlash(path1);
    addLastSlash(path2);
    if (path1.length() != path2.length())
        return false;

    slashReplace(path1);
    slashReplace(path2);
    return equalStrIC(path1, path2);
}


#ifdef Q_OS_WIN

QString expandEnvironmentStrings(const QString& s)
{
    #define INFO_BUFFER_SIZE 32767
    wchar_t infoBuf[INFO_BUFFER_SIZE] = {0};
    DWORD bufCharCount = ExpandEnvironmentStringsW((LPCWSTR)s.utf16(), infoBuf, INFO_BUFFER_SIZE);
    return (bufCharCount < INFO_BUFFER_SIZE) ? QString::fromWCharArray(infoBuf) : QString();
}

QString systemRoot()
{
	static QString system_root(expandEnvironmentStrings("%SystemRoot%"));
    return system_root;
}

QString tempDir()
{
	//static QString temp_dir(expandEnvironmentStrings("%Temp%"));
    return expandEnvironmentStrings("%Temp%");
}

QString systemDisk()
{
	static QString system_sisk(systemRoot().mid(0, 3));
    return system_sisk;
}

//GUID createGUID(bool *ok /*= NULL*/)
//{
//    GUID guid;
//    bool res = createGUID(guid);
//    if (ok) *ok = res;
//    return guid;
//}

//bool guidToString(const GUID &guid, QString &s)
//{
//    wchar_t* str_guid;
//    HRESULT res = ::StringFromCLSID(guid, &str_guid);
//    s = QString::fromWCharArray(str_guid);
//    CoTaskMemFree(str_guid);
//    return (res == S_OK);
//}

//QString guidToString(const GUID &guid, bool *ok /*= NULL*/)
//{
//    QString s;
//    bool res = guidToString(guid, s);
//    if (ok) *ok = res;
//    return s;
//}

//bool createStringGUID(QString &s)
//{
//    GUID guid;
//    bool res = createGUID(guid);
//    if (res)
//        res = guidToString(guid, s);

//    return res;
//}

//QString createStringGUID(bool *ok /*= NULL*/)
//{
//    QString s;
//    bool res = createStringGUID(s);
//    if (ok) *ok = res;
//    return s;
//}

QString moduleFileName(HINSTANCE instance)
{
    QString module_file_name;
    ulong buff_size = MAX_PATH;
    while (true)
    {
        //sModuleFileName.SetLength( dwBuffSize );
        module_file_name.reserve(buff_size);
        ulong result = GetModuleFileNameW(instance, (TCHAR*) module_file_name.utf16(), buff_size);
        if (result > buff_size) {
            buff_size = buff_size + MAX_PATH;
            continue;
        }
        module_file_name.resize(result);
        break;  
    }
    //if ( IsTrEmpty( sModuleFileName ))
    //  throw Exception( FUNC_ERR( "Ошибка получения имени модуля." ));

    return module_file_name;
}

#endif //Q_OS_WIN


QString fileExtension(const QString& fileName)
{
    QString file_ext;
    //QString file_name = fileName;
    if (fileName.length()) {
        // !!! Отладить !!!
        //_CrtDbgBreak();
        int ind = fileName.lastIndexOf(QChar('.'));
        if (ind > 0) 
            file_ext = fileName.mid(ind + 1);
    }
    return file_ext;    
}

QString replaceFileExtension(const QString& fileName, const QString& newExt)
{
    //_CrtDbgBreak();

    QString file_name = fileName;
    if (file_name.length()) {
        int ind = file_name.lastIndexOf(QChar('.'));
        if (ind > 0) 
            file_name.resize(ind);
        file_name += newExt;
    }
    return file_name;
}


#define SLASH_PREPEND "\\\\?\\"

bool deleteFile(QString fileName)
{
    //_CrtDbgBreak();

#ifdef Q_OS_WIN
    //slashReplace(fileName, '/', '\\');
    toNativeSlash(fileName);
    if (!fileName.startsWith(SLASH_PREPEND))
        //fileName = "\\\\?\\" + fileName;
        fileName.prepend(SLASH_PREPEND);
    return ::DeleteFileW((LPCWSTR)fileName.utf16());
#else
    #error "No implement"
#endif
}

bool deleteDirectory(QString dirName)
{
    //_CrtDbgBreak();

#ifdef Q_OS_WIN
    //slashReplace(dirName, '/', '\\');
    toNativeSlash(dirName);
    if (!dirName.startsWith(SLASH_PREPEND))
        //dirName = "\\\\?\\" + dirName;
        dirName.prepend(SLASH_PREPEND);
    return ::RemoveDirectoryW((LPCWSTR)dirName.utf16());
#else
    #error "No implement"
#endif
}

bool pathFileExists(QString path)
{
    //_CrtDbgBreak();

#ifdef Q_OS_WIN
    //slashReplace(path, '/', '\\');
    toNativeSlash(path);
    if (!path.startsWith(SLASH_PREPEND))
        //path = "\\\\?\\" + path;
        path.prepend(SLASH_PREPEND);
    return ::PathFileExistsW((LPCWSTR)path.utf16());
#else
    #error "No implement"
#endif
}

bool isDirectory(QString path)
{
    //_CrtDbgBreak();

#ifdef Q_OS_WIN
    //slashReplace(path, '/', '\\');
    toNativeSlash(path);
    if (!path.startsWith(SLASH_PREPEND))
        //path = "\\\\?\\" + path;
        path.prepend(SLASH_PREPEND);
    DWORD attr = ::GetFileAttributesW((LPCWSTR)path.utf16());
    return (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    #error "No implement"
#endif
}
#undef SLASH_PREPEND


QString applPath(const QString& fileName)
{
    QString path = qApp->applicationDirPath();
    if (path.endsWith("release", Qt::CaseInsensitive))
        path.chop(8);
    else if (path.endsWith("debug", Qt::CaseInsensitive))
        path.chop(6);

    if (fileName.length()) {
        addLastSlash(path);
        path += fileName;
    }
    return toNativeSlash(path);
}


#endif //QT_VERSION

}; /*namespace futl*/
