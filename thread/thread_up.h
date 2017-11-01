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
*****************************************************************************/

#pragma once

#include "thread_base.h"
#include <string>

namespace trd {

/**
  В классе реализован механизм повышения приоритета потока
*/
class ThreadUp : public ThreadBase
{
public:
    // Функция устанавливает тип и приоритет потока. В случае успешного выполне-
    // ния возвращает 0, иначе - код ошибки. Строковое описание кода ошибки можно
    // получить из функции priorityUpError().
    // Параметр schedulAlgorithm может принимать одно из следующих значений:
    // SCHED_OTHER, SCHED_FIFO, SCHED_RR (см. http://man.sourcentral.org/f17/ru/2+sched_setscheduler)
    // Значения параметра priority зависят от schedulAlgorithm.
    int priorityUp(int schedulAlgorithm, int priority);

    // Строковое описание ошибки
    std::string priorityUpError() const {return _priorityUpError;}

private:
    std::string _priorityUpError;
};

} //namespace trd
