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

#include "clife_base.h"
#include <type_traits>

/**
  clife_alloc - вспомогательный класс, используется в контейнере lst::List
  для создания и разрушения элементов списка унаследованных от clife_base
*/
template<typename T> struct clife_alloc
{
    T* create() {
        static_assert(std::is_base_of<clife_base, T>::value,
                      "Type T must be derived from clife_base");
        T* x = new T();
        x->add_ref();
        return x;
    }
    void destroy(clife_base* x) {if (x) x->release();}
};
