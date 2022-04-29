/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2018 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  В модуле реализован простой кольцевой буфер. Буфер позволяет выполнять
  операции чтения/записи данных произвольной длинны
*****************************************************************************/

#pragma once
#include <mutex>

class RingBuffer
{
public:
    RingBuffer() = default;
    ~RingBuffer();

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator= (RingBuffer&&) = delete;
    RingBuffer& operator= (const RingBuffer&) = delete;

    // Выполняет инициализацию буфера и устанавливает его размер
    void init(size_t bufferSize);

    // Обнуляет буфер. После вызова этой функции операции чтения/записи станут
    // невозможны
    void reset();

    // Возвращает количество данных в буфере доступных для чтения
    size_t available() const;

    // Возвращает размер буфера
    size_t size() const;

    // Выполняет чтение данных из буфера. Размер читаемых данных определяется
    // параметром length. Результат записывается в параметр dest. Память для
    // dest должна быть выделена заранее и не должна быть меньше length.
    // Если значение length больше размера доступных для чтения данных - функция
    // вернет FALSE, и данные не будут записаны в dest
    bool read(char* dest, size_t length);

    // Выполняет запись данных в буфер. Размер записываемых данных определяется
    // параметром length. Значение параметра length не должно превышать половины
    // размера буфера, в противном случае данные не будут записаны в буфер и
    // функция вернет FALSE
    bool write(const char* source, size_t length);

private:
    char* _begin = {nullptr}; // Позиция из которой выполняется чтение данных
    char* _end   = {nullptr}; // Позиция в которую выполняется запись данных

    char* _buffBegin = {nullptr}; // Начало буфера данных
    char* _buffEnd   = {nullptr}; // Указатель на адрес идущий за последним
                                  // элементом буфера данных
    size_t _available = {0};
    size_t _buffSize  = {0};
    mutable std::mutex _buffLock;
};

