/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2026 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include "qt/quuidex.h"
#include <QtCore>
#include <tuple>

/**
  Структура используется для идентификации различных примитивов. Идентификация
  может осуществляться по одному или паре полей:  id,  index,  name.  За выбор
  полей для идентификации примитива отвечает пользователь API
*/
struct Identity
{
    Identity() = default;
    Identity(const Identity&) = default;

    Identity(const QUuidEx& id  ) : id(id),       findField(FindField::Id)    {}
    Identity(const int index    ) : index(index), findField(FindField::Index) {}
    Identity(const QString& name) : name(name),   findField(FindField::Name)  {}
    Identity(const char*    name) : name(name),   findField(FindField::Name)  {}

    Identity(const QUuidEx& id, int index)
        : id(id), index(index), findField(FindField::IdIndex)
    {}

    Identity(const QUuidEx& id, const QString& name)
        : id(id), name(name), findField(FindField::IdName)
    {}
    Identity(const QUuidEx& id, const char* name)
        : id(id), name(name), findField(FindField::IdName)
    {}

    Identity(int index, const QString& name)
        : index(index), name(name), findField(FindField::IndexName)
    {}
    Identity(int index, const char* name)
        : index(index), name(name), findField(FindField::IndexName)
    {}

    QUuidEx id;
    qint32  index = {-1};
    QString name;

    // Определяет поля по которым нужно выполнять поиск/идентификацию
    enum class FindField : quint32
    {
        Id        = 0, // По полю id
        Index     = 1, // По полю index
        Name      = 2, // По полю name
        IdIndex   = 3, // По полям id,index
        IdName    = 4, // По полям id,name
        IndexName = 5  // По полям index,name
    };
    FindField findField = {FindField::Index};

    template<typename T> struct Find
    {
        // FindField::Id
        int operator() (const QUuidEx* id, const T* item2) const
            {return QUuidEx::compare(*id, item2->identity.id);}

        // FindField::Index
        int operator() (const qint32* index, const T* item2) const
            {return LIST_COMPARE_ITEM(*index, item2->identity.index);}

        // FindField::Name
        int operator() (const QString* name, const T* item2) const
            {return name->compare(item2->identity.name);}

        // FindField::IdIndex
        int operator() (const std::tuple<QUuidEx /*id*/, qint32 /*index*/>* item1,
                        const T* item2) const
        {
            if (int res = QUuidEx::compare(std::get<0>(*item1), item2->identity.id))
                return res;
            return LIST_COMPARE_ITEM(std::get<1>(*item1), item2->identity.index);
        }

        // FindField::IdName
        int operator() (const std::tuple<QUuidEx /*id*/, QString /*name*/>* item1,
                        const T* item2) const
        {
            if (int res = QUuidEx::compare(std::get<0>(*item1), item2->identity.id))
                return res;
            return QString::compare(std::get<1>(*item1), item2->identity.name);
        }

        // FindField::IndexName
        int operator() (const std::tuple<qint32 /*index*/, QString /*name*/>* item1,
                        const T* item2) const
        {
            LIST_COMPARE_MULTI_ITEM(std::get<0>(*item1), item2->identity.index);
            return QString::compare(std::get<1>(*item1), item2->identity.name);
        }

        int operator() (const Identity* item1, const T* item2) const
        {
            if (item1->findField == FindField::Id) {
                return operator() (&item1->id, item2);
            }
            else if (item1->findField == FindField::Index) {
                return operator() (&item1->index, item2);
            }
            else if (item1->findField == FindField::Name) {
                return operator() (&item1->name, item2);
            }
            else if (item1->findField == FindField::IdIndex) {
                std::tuple t {item1->id, item1->index};
                return operator() (&t, item2);
            }
            else if (item1->findField == FindField::IdName) {
                std::tuple t {item1->id, item1->name};
                return operator() (&t, item2);
            }
            else if (item1->findField == FindField::IndexName) {
                std::tuple t {item1->index, item1->name};
                return operator() (&t, item2);
            }
            return -1;
        }
    };

    bool isNull() const;
    QString toString() const;
};
