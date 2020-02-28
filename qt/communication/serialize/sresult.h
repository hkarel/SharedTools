/*****************************************************************************
  The MIT License

  Copyright © 2019 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "container_ptr.h"
#include <QtCore>
#include <utility>

namespace communication {
namespace serialization {

/**
  Структура используется для возвращения результата работы функций
  сериализации/десериализации данных
*/
class Result
{
public:
    Result() = default;
    Result(bool, int code = 0, const QString& description = QString());

    Result(const Result&) = default;
    Result& operator = (const Result&) = default;

    Result(Result&&);
    Result& operator= (Result&&);

    explicit operator bool () const {return _d->value;}

    // Код ошибки
    qint32 code() const {return _d->code;}

    // Описание ошибки
    QString description() const {return _d->description;}

private:
    struct Data
    {
        typedef container_ptr<Data> Ptr;

        bool    value = {true};
        qint32  code  = {0};
        QString description;
    };
    Data::Ptr _d = {Data::Ptr::create_join_ptr()};
};

} // namespace serialization

using SResult = serialization::Result;

} // namespace communication
