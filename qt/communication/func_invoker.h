/*****************************************************************************
  В модуле реализована структура, используемая для связи команды и функции-
  обработчика сообщения содержащего данную команду.
*****************************************************************************/

#pragma once

#include "_list.h"
#include "break_point.h"
#include "qt/communication/message.h"

namespace communication {

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
        _functions.add(item);
    }

    template<typename T>
    void registration(const QUuidEx& command,
                      void (T::*func)(const Message::Ptr&) const, T* instance)
    {
        ItemConst<T>* item = new ItemConst<T>;
        item->command = command;
        item->instance = instance;
        item->func = func;
        _functions.add(item);
    }

    void sort()
    {
        _functions.sort(lst::SortUp);
    }

    bool containsCommand(const QUuidEx& command)
    {
        checkFunctionsSort();
        return _functions.findRef(command).success();
    }

    void call(const Message::Ptr& message)
    {
        checkFunctionsSort();
        if (lst::FindResult fr = _functions.findRef(message->command()))
            _functions.item(fr.index())->call(message);
    }

private:
    void checkFunctionsSort()
    {
#ifndef NDEBUG
        if (_functions.sortState() != lst::SortUp)
        {
            break_point
            // --- Напоминание о необходимости сортировать список функций ---
            // После регистрации всех функций-обработчиков сообщений необходимо
            // отсортировать список функций-обработчиков. Для этого нужно явно
            // вызвать метод FunctionInvoker::sort()
        }
    }
#endif

    BaseItem::List _functions;
};

} // namespace communication
