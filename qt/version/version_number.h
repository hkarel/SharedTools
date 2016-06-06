/*****************************************************************************
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

        };
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

// Минимальная версия для которой еще обеспечивается совместимость
// бинарного протокола
const VersionNumber& minCompatibleVersion();
