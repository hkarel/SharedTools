/* clang-format off */
/*****************************************************************************
  В модуле собраны сервисные функции общего назначения.

*****************************************************************************/

#pragma once

#include <sys/types.h>
#include <stdio.h>
#include <atomic>
#include <string>
#include <cstdarg>
#include <streambuf>
#include <vector>
#include <algorithm>

namespace utl {

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
string formatMessage(const char* format, ...)  __attribute__ ((format (printf, 1, 2)));

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
string toString(short val);
string toString(unsigned short val);

string toString(int val);
string toString(unsigned int val);

string toString(long val);
string toString(unsigned long val);

string toString(long long val);
string toString(unsigned long long val);


// Выполняет преобразование UUID в строковое представление.
string uuidToString(const uint8_t uuid[16]);

// Аналогична функции uuidToString(const uint8_t uuid[16]), но результат пишется
// в параметр result.
void uuidToString(const uint8_t uuid[16], uint8_t result[40]);

// Выполняет преобразование UUID в шестнадцатеричное строковое представление.
// Параметр addHexPrefix определяет будет ли в начало результирующей строки
// добавлен признак шестнадцатеричного представления '0x'.
string uuidToHexString(const uint8_t uuid[16], bool addHexPrefix = true);

// Аналогична функции uuidToHexString(const uint8_t uuid[16], bool addHexPrefix),
// но результат пишется в параметр result.
void uuidToHexString(const uint8_t uuid[16], uint8_t result[40], bool addHexPrefix);

// Разделяет строку на список строк согласно указанному разделителю.
//   str - исходная строка;
//   delim - разделитель;
//   keepEmptyParts - сохранять в результирующем списке пустые строки.
vector<string> split(const string& str, char delim, bool keepEmptyParts = false);

// Выполняет округление действительного числа number до signCount знаков
// после запятой
double round(double number, int signCount);

// Возвращает среднее арифметическое для списка элементов
template<typename T>
double average(const T& elements);

//----------------------------- Implementation -------------------------------

template<typename std::size_t BuffSize>
string formatMessage(const char* format, ...)
{
    char buff[BuffSize] = {0};
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(buff, BuffSize - 1, format, argptr);
    va_end(argptr);
    return buff;
}

template<typename T>
double average(const T& elements)
{
    double sum = std::accumulate(elements.begin(), elements.end(), typename T::value_type(0));
    return (sum / elements.size());
}

} // namespace utl
