/****************************************************************************
  В модуле собраны сервисные функции общего назначения.

****************************************************************************/

#pragma once

#include <atomic>
#include <string>
#include <cstdarg>
#include <streambuf>


namespace utl
{
using namespace std;

/**
  Сервисная структура, позволяет ассоциировать линейный буфер в памяти
  с потоковыми механизмами.
*/
struct MemBuff : std::streambuf
{
    MemBuff(char* base, size_t size)
    {
        //char* p((char*>(base));
        this->setg(base, base, base + size);
    }
};

// Функции удаления пробелов в начале и конце строки
string& ltrim(string&);
string& rtrim(string&);
string& trim(string&);

// Устанавливает текущую директорию по имени исполняемого файла.
bool setCurrentDir(char* binaryPath);

// Записывает PID в файл.
void savePidFile(const string& fileName);

// Аналог функции sprintf, в качестве результата возвращает отформатированную
// строку
template<typename std::size_t BuffSize = 1024>
string formatMessage(const char* format, ...)
{
    char buff[BuffSize] = {0};
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(buff, BuffSize - 1, format, argptr);
    va_end(argptr);
    return buff;
}

// Создает все вложенные директории для указанного пути. Параметр throwExcept
// определяет будет ли выбрасываться исключение boost::system::system_error
// в случае невозможности создать диреторию.
bool mkDir(const string& dir, bool throwExcept = false);

// Используется для изменения значений атомарных флагов. Функция используется
// в тех случаях, когда нужно избегать постоянного присвоения значений атомар-
// ной переменной. В примере ниже в переменной atomic_val происходит постоянное
// присвоение значения true. Каждое присвоение приводик к сбросу линии кеша
// процессора, что понижает производительность.
// while (true) {
//    ...
//    if (true)
//        atomic_val = true
// }
// Функция позволяет избежать постоянных присвоений.
inline bool assign(atomic_bool& a, bool value)
{
    bool b = !value;
    return a.compare_exchange_strong(b, value);
}

// Выполняет преобразование в строку
string toString(int val);

} // namespace utl
