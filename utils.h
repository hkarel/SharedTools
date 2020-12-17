/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2013 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  В модуле собраны сервисные функции общего назначения.

*****************************************************************************/

#pragma once

#include <sys/types.h>
#include <sys/time.h>
#include <atomic>
#include <string>
#include <cstdarg>
#include <streambuf>
#include <vector>
#include <numeric>
#include <algorithm>

#if __cplusplus >= 201703L
#include <charconv>
#endif

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

// Функция изменяет значений атомарных флагов,  используется  в  тех  случаях,
// когда нужно избегать постоянного присвоения значений атомарной  переменной.
// В примере ниже в переменной atomic_val  происходит  постоянное  присвоение
// значения true. Каждое присвоение приводик к сбросу линии кеша процессора,
// что снижает производительность.
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
#if __cplusplus >= 201703L
template<typename T>
string toString(T val, enable_if_t<is_integral<T>::value, int> = 0)
{
    char buff[32];
    to_chars_result res = to_chars(buff, buff + sizeof(buff), val);
    if (res.ec == std::errc())
    {
        *res.ptr = '\0';
        return buff;
    }
    return {};
}
#else
string toString(short val);
string toString(unsigned short val);

string toString(int val);
string toString(unsigned int val);

string toString(long val);
string toString(unsigned long val);

string toString(long long val);
string toString(unsigned long long val);
#endif

// Выполняет преобразование UUID в строковое представление.
string uuidToString(const uint8_t uuid[16]);

// Аналогична функции uuidToString(const uint8_t uuid[16]), но результат пишется
// в параметр result.
void uuidToString(const uint8_t uuid[16], uint8_t result[40]);

// Выполняет преобразование UUID в шестнадцатеричное строковое представление.
// Параметр addHexPrefix определяет будет ли в начало результирующей строки
// добавлен признак шестнадцатеричного представления '0x'.
string uuidToHexString(const uint8_t uuid[16], bool addHexPrefix = true);

// Аналог функции uuidToHexString(const uint8_t uuid[16], bool addHexPrefix),
// но результат пишется в параметр result.
void uuidToHexString(const uint8_t uuid[16], uint8_t result[40], bool addHexPrefix);

// Разделяет строку на список строк согласно указанному разделителю.
//   str - исходная строка;
//   delim - разделитель;
//   keepEmptyParts - сохранять в результирующем списке пустые строки.
vector<string> split(const string& str, char delim, bool keepEmptyParts = false);

// Выполняет примитивное (не математическое) округление действительного числа
// number до signCount знаков после запятой
double round(double number, int signCount);

// Функции сложения/вычитания структур timeval. Они повторяют макросы
// timeradd/timersub, но в MinGW эти макросы не реализованы.
void timeAdd(const timeval& a, const timeval& b, timeval& result);
void timeSub(const timeval& a, const timeval& b, timeval& result);

// Возвращает сумму элементов списка
template<typename T>
typename T::value_type sum(const T& elements);

// Возвращает среднее арифметическое элементов списка
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
typename T::value_type sum(const T& elements)
{
    return std::accumulate(elements.begin(), elements.end(), typename T::value_type(0));
}

template<typename T>
double average(const T& elements)
{
    double s = sum(elements);
    return (s / elements.size());
}

} // namespace utl
