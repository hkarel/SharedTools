/*****************************************************************************
  The MIT License

  Copyright © 2017 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  Модуль содержит механизмы сериализации в json формат.
*****************************************************************************/

#pragma once

#include "qt/communication/message.h"
#include <QtCore>
#include <string>

#ifndef JSON_SERIALIZATION
#error "Is not defined mechanism for json  serialization"
#endif
#include "rapidjson/document.h"

namespace communication {
namespace serialization {
namespace json {

using namespace rapidjson;

template <typename GenericValueT>
bool stringEqual(const typename GenericValueT::Ch* a, const GenericValueT& b)
{
    RAPIDJSON_ASSERT(b.IsString());

    const SizeType l1 = strlen(a);
    const SizeType l2 = b.GetStringLength();
    if (l1 != l2)
        return false;

    const typename GenericValueT::Ch* const b_ = b.GetString();
    if (a == b_)
        return true;

    return (std::memcmp(a, b_, sizeof(typename GenericValueT::Ch) * l1) == 0);
}


} // namespace json
} // namespace serialization
} // namespace communication
