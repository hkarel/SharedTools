/*****************************************************************************
  The MIT License

  Copyright © 2015 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  Модуль описывает функции и структуры отвечающие за контроль версий проекта.

*****************************************************************************/

#pragma once

#include <QtGlobal>
#include <QString>

struct VersionNumber
{
    union {
        quint32 vers;       // Представление версии в 32-битной форме
        struct {
            quint8 major;   // Старший номер версии
            quint8 minor;   // Младший номер версии
            quint8 patch;   // Номер исправления
            quint8 build;   // Зарезервировано

        } ver;
    };
    VersionNumber() : vers(0) {}
    VersionNumber(quint32 vers) : vers(vers) {}
    VersionNumber(quint8 major, quint8 minor, quint8 patch);

    // Возвращает версию в формате 'major.minor.patch'
    QString toString() const;
};

bool operator== (const VersionNumber, const VersionNumber);
bool operator!= (const VersionNumber, const VersionNumber);
bool operator>  (const VersionNumber, const VersionNumber);
bool operator<  (const VersionNumber, const VersionNumber);
bool operator>= (const VersionNumber, const VersionNumber);
bool operator<= (const VersionNumber, const VersionNumber);

// Текущая версия продукта
const VersionNumber& productVersion();
