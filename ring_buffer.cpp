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
*****************************************************************************/

#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h>

RingBuffer::~RingBuffer()
{
    reset();
}

void RingBuffer::init(size_t bufferSize)
{
    std::lock_guard<std::mutex> locker {_buffLock}; (void) locker;

    if (_buffBegin)
        free(_buffBegin);

    _buffSize  = bufferSize;
    _buffBegin = (char*) malloc(_buffSize);
    _buffEnd   = _buffBegin + _buffSize;
    _begin     = _buffBegin;
    _end       = _buffBegin;
    _available = 0;

#ifndef NDEBUG
    memset(_buffBegin, 0, bufferSize);
#endif
}

void RingBuffer::reset()
{
    std::lock_guard<std::mutex> locker {_buffLock}; (void) locker;

    if (_buffBegin)
        free(_buffBegin);

    _begin     = nullptr;
    _end       = nullptr;
    _buffBegin = nullptr;
    _buffEnd   = nullptr;
    _buffSize  = 0;
    _available = 0;
}

size_t RingBuffer::available() const
{
    std::lock_guard<std::mutex> locker {_buffLock}; (void) locker;
    return _available;
}

size_t RingBuffer::size() const
{
    std::lock_guard<std::mutex> locker {_buffLock}; (void) locker;
    return _buffSize;
}

bool RingBuffer::read(char* dest, size_t length)
{
    std::lock_guard<std::mutex> locker {_buffLock}; (void) locker;

    if (_buffBegin == nullptr
        || _available < length)
    {
        return false;
    }

    if ((_begin + length) <= _buffEnd)
    {
        memcpy(dest, _begin, length);
        _begin += length;

        if (_begin == _buffEnd)
            _begin = _buffBegin;
    }
    else
    {
        size_t l1 = _buffEnd - _begin;
        size_t l2 = length - l1;

        memcpy(dest, _begin, l1);
        memcpy(dest + l1, _buffBegin, l2);

        _begin = _buffBegin + l2;
    }
    _available -= length;
    return true;
}

bool RingBuffer::write(const char* source, size_t length)
{
    std::lock_guard<std::mutex> locker {_buffLock}; (void) locker;

    if (_buffBegin == nullptr
        || (length * 2) > _buffSize)
    {
        return false;
    }

    if (_begin <= _end)
    {
        if ((_end + length) <= _buffEnd)
        {
            // Условие W1 (метка для тестов)
            memcpy(_end, source, length);
            _end += length;
        }
        else
        {
            // Условие W2 (метка для тестов)
            size_t l1 = _buffEnd - _end;
            size_t l2 = length - l1;

            memcpy(_end, source, l1);
            memcpy(_buffBegin, source + l1, l2);

            _end = _buffBegin + l2;
            if (_begin < _end)
                _begin = _end;
        }
    }
    else // _end < _begin
    {
        if ((_end + length) <= _buffEnd)
        {
            // Условие W3 (метка для тестов)
            memcpy(_end, source, length);

            _end += length;
            if (_begin < _end)
                _begin = _end;
        }
        else
        {
            // Условие W4 (метка для тестов)
            size_t l1 = _buffEnd - _end;
            size_t l2 = length - l1;

            memcpy(_end, source, l1);
            memcpy(_buffBegin, source + l1, l2);

            _end = _buffBegin + l2;
            _begin = _end;
        }
    }
    if (_begin == _buffEnd)
        _begin = _buffBegin;

    if (_end == _buffEnd)
        _end = _buffBegin;

    _available += length;
    if (_available > _buffSize)
        _available = _buffSize;

    return true;
}
