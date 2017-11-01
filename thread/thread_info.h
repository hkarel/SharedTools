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

  В модуле реализованы вспомогательные функции для работы с потоками.
  Основанием для создания этого модуля явилась некорректная работа функции
  pthread_kill() при определении статуса завершенного потока в Lunux-системах,
  что приводит к SIGABRT. А так же упорное нежелание автора функции менять ее
  поведение. В качестве причины отказа называется описание в POSIX-стандарте,
  и что поведение функции соответствует заявленным требованиям.
  Примечание: для BSD - систем функция не вызывает падения и корректно возвра-
              щает статус завершенного потока.
*****************************************************************************/

#pragma once
#include <sys/types.h>

namespace trd {

// Возвращает идентификатор (TID) потока
pid_t gettid();

// Возвращает TRUE если поток с указанным идентификатором существует
bool thread_exists(pid_t tid);

} // namespace trd
