/* clang-format off */
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

#pragma once
#include <atomic>

/**
  clife_base - базовый класс для управления жизнью объекта с использованием
  внутреннего счетчика ссылок.  Предполагается,  что классы унаследованные
  от clife_base будут использоваться совместно со смарт-указателем clife_ptr
*/
class clife_base
{
public:
    // Предполагается, что основная работа с объектом clife_base будет происхо-
    // дить через clife_ptr, в этом случае  при  создании  объекта  clife_base
    // предпочтительно счетчик жизни объекта устанавливать в 0 (по умолчанию).
    // При получении объекта clife_ptr увеличит счетчик жизни объекта в своем
    // конструкторе.
    // Если же при создании объекта clife_base не планируется его немедленная
    // передача во владение clife_ptr, то целесообразно счетчик жизни устанав-
    // ливать в 1. В этом случае в коде будет меньше вызовов метода add_ref()
    clife_base() = default;
    clife_base(bool add_ref) : _clife_count(add_ref ? 1 : 0) {}

    // Конструктор и оператор присваивания сделаны  фиктивными  для  того,
    // чтобы предотвратить изменение счетчика жизни объекта (_clife_count)
    // вне функций add_ref()/release()
    clife_base(const clife_base&) {}
    clife_base& operator= (const clife_base&) {return *this;}

    clife_base(clife_base&&) = delete;
    clife_base& operator= (clife_base&&) = delete;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"

    virtual ~clife_base() = default;

#pragma GCC diagnostic pop

    void add_ref() const {++_clife_count;}
    void release() const {if (--_clife_count == 0) delete this;}

    uint32_t clife_count() const {return _clife_count;}

private:
    mutable std::atomic<uint32_t> _clife_count = {0};
};
