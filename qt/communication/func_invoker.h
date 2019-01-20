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
*****************************************************************************/

#pragma once

#include "list.h"
#include "break_point.h"
#include "qt/communication/message.h"

namespace communication {

/**
  В классе реализован механизм для связывания команды с функцией-обработчиком
  сообщения.
*/
class FunctionInvoker
{
    struct BaseItem
    {
        QUuidEx command;
        virtual ~BaseItem() = default;
        virtual void call(const Message::Ptr&) const = 0;

        struct Compare
        {
            int operator() (const BaseItem* item1, const BaseItem* item2, void*) const
            {
                return QUuidEx::compare(item1->command, item2->command);
            }
            int operator() (const QUuidEx* command, const BaseItem* item2, void*) const
            {
                return QUuidEx::compare(*command, item2->command);
            }
        };
        typedef lst::List<BaseItem, Compare> List;
    };

    template<typename T>
    struct Item : BaseItem
    {
        typedef void (T::*Func)(const Message::Ptr&);
        T* instance;
        Func func;
        void call(const Message::Ptr& message) const
        {
            (instance->*func)(message);
        }
    };

    template<typename T>
    struct ItemConst : BaseItem
    {
        typedef void (T::*Func)(const Message::Ptr&) const;
        T* instance;
        Func func;
        void call(const Message::Ptr& message) const
        {
            (instance->*func)(message);
        }
    };

public:
    template<typename T>
    void registration(const QUuidEx& command,
                      void (T::*func)(const Message::Ptr&), T* instance)
    {
        Item<T>* item = new Item<T>;
        item->command = command;
        item->instance = instance;
        item->func = func;

        if (lst::FindResult fr = _functions.find(item))
            _functions.remove(fr.index());

        _functions.add(item);
        _functions.sort();
    }

    template<typename T>
    void registration(const QUuidEx& command,
                      void (T::*func)(const Message::Ptr&) const, T* instance)
    {
        ItemConst<T>* item = new ItemConst<T>;
        item->command = command;
        item->instance = instance;
        item->func = func;

        if (lst::FindResult fr = _functions.find(item))
            _functions.remove(fr.index());

        _functions.add(item);
        _functions.sort();
    }

    bool containsCommand(const QUuidEx& command)
    {
        return _functions.findRef(command).success();
    }

    lst::FindResult findCommand(const QUuidEx& command)
    {
        return _functions.findRef(command);
    }

    void call(const Message::Ptr& message)
    {
        if (lst::FindResult fr = _functions.findRef(message->command()))
            _functions.item(fr.index())->call(message);
    }

    void call(const Message::Ptr& message, const lst::FindResult& fr)
    {
        if (fr.success())
            _functions.item(fr.index())->call(message);
    }

private:
    BaseItem::List _functions;
};

} // namespace communication
