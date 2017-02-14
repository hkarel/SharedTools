/* clang-format off */
/*****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  Вспомогательный класс, используется в контейнере lst::List для создания и
  разрушения элементов списка унаследованных от clife_base.
*****************************************************************************/

#pragma once
#include "clife_base.h"
#include <type_traits>

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
