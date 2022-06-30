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

  В модуле реализован простейший событийный механизм типа signal/slot. В каче-
  стве слотов могут использоваться  как обычные  функции, так и функции члены
  классов. Связывание слота и функции выполняется через механизм CLOSURE.
  Пример использования:
  struct A
  {
      void b(float) {std::count << "Call b() \n";}
      void c(float) {std::count << "Call c() \n";}
  };

  A a;
  typedef Closure<void (float)> Func;
  Func fb = CLOSURE(&A::b, &a);
  Func fc = CLOSURE(&A::c, &a);

  SimpleSignal<Func> signal;
  signal.connect(fb);
  signal.connect(fc);

  signal2.emit_(0);
  ---

  Вывод:
  Call b()
  Call с()

  Предполагается, что если слот связан с функцией-членом класса, то при разру-
  шении объекта данного класса  необходимо  выполнить  процедуру  отсоединения
  слота.
  Для примера выше это будет вызов:
    signal.disconnect(fb);
    signal.disconnect(fc);

  Примечание: качестве слотов не могут использоваться  функции  с  сигнатурами
  содержащими  rvalue-параметры.  При  передаче  rvalue-параметров  в  список
  слотов-обработчиков корректным будет  вызов  только  первого  слота,  а  все
  последующие  слоты  будут  получать  невалидные  значения  rvalue-параметров.
  По этой причине в функциях emit_() и call() при передаче  параметров  args...
  не используется вызов функции std::forward()
*****************************************************************************/

#pragma once

#include "closure/closure3.h"
#include "spin_locker.h"

#include <atomic>
#include <vector>
#include <type_traits>

/**
  SimpleSignalBase - базовая реализация механизма SIGNAL
*/
template<
    typename R,
    typename... Args
>
class SimpleSignalBase
{
    // sfinae types (взято из std::function)
    template<typename Res>
    struct enable_if_void : std::enable_if<std::is_void<Res>::value, int> {};
    template<typename Res>
    struct disable_if_void : std::enable_if<!std::is_void<Res>::value, int> {};

public:
    typedef Closure<R (Args...)>  Slot;
    typedef std::vector<Slot>     SlotList;

    // Подключить обработчик. Если параметр unique = FALSE это позволит
    // подключать обработчик f повторно
    void connect(const Slot& f, bool unique = true);

    // Отключить обработчик. Если параметр all = TRUE, то будут отключены
    // все обработчики f
    void disconnect(const Slot& f, bool all = true);

    // Проверяет наличие обработчика в списке слотов
    bool exists(const Slot& f) const;

    // Возвращает TRUE если список обработчиков пуст
    bool empty() const;

    // Выполняет вызов связанных обработчиков.
    // Примечание: порядок вызова обработчиков может быть не определен
    R emit_(Args... args) const;

    // Блокирует вызов обработчиков
    void block(bool val) {_block = val;}

    // Признак блокировки вызова обработчиков
    bool blocked() const {return _block;}

    // Очистить список обработчиков
    void clear();

protected:
    SimpleSignalBase() = default;

private:
    SimpleSignalBase(const SimpleSignalBase &) = delete;
    SimpleSignalBase& operator= (const SimpleSignalBase &) = delete;

    SimpleSignalBase(SimpleSignalBase &&) = delete;
    SimpleSignalBase& operator= (SimpleSignalBase &&) = delete;

    template<typename T>
    struct ResetSlotIndex
    {
        T* p;
        ResetSlotIndex(T* p) noexcept : p(p) {}
        ~ResetSlotIndex() noexcept {(*p) = -1;}
    };

    template<typename Res>
    Res call(Args... args, typename disable_if_void<Res>::type = 0) const;

    template<typename Res>
    void call(Args... args, typename enable_if_void<Res>::type = 0) const;

private:
    // Используется для запрета одновременного вызова метода emit()  из  разных
    // потоков. Запрет носит не технический, а концептуальный характер: эмиссия
    // сообщений не должна производиться произвольно из разных потоков, так как
    // это будет вносить хаос в рассылку сообщений.
    // [11.10.2016] Карелин: использование мьютекса снижает производительность
    //              вызовов событий более чем в 2 раза.
    // mutable std::mutex _emitLock;

    SlotList _slots;
    mutable std::atomic_flag _slotsLock = ATOMIC_FLAG_INIT;

    // Содержит индекс слота из списка _slots вызываемого в данный момент
    // в функции call()
    // [11.10.2016] Карелин: использование atomic_int катастрофически
    //              снижает производительность вызовов call().
    // mutable std::atomic_int _slotIndex = {-1};
    mutable std::size_t _slotIndex = std::size_t(-1);

    volatile bool _block = {false};
};


// Обобщенная декларация
template<typename, typename... > struct SimpleSignal;

/**
  Специализации для SimpleSignal
*/
template<typename R, typename... Args>
struct SimpleSignal<Closure<R (Args...)>> : public SimpleSignalBase<R, Args...>
{
    typedef typename SimpleSignalBase<R, Args...>::Slot     Slot;
    typedef typename SimpleSignalBase<R, Args...>::SlotList SlotList;

    void test() {}
};

template<typename R, typename... Args>
struct SimpleSignal<R (Args...)> : public SimpleSignalBase<R, Args...>
{
    typedef typename SimpleSignalBase<R, Args...>::Slot     Slot;
    typedef typename SimpleSignalBase<R, Args...>::SlotList SlotList;

    void test() {}
};

//------------------------------ Implementation ------------------------------

template<typename R, typename... Args>
void SimpleSignalBase<R, Args...>::connect(const Slot& f, bool unique)
{
    SpinLocker locker {_slotsLock}; (void) locker;
    if (unique)
        for (const Slot& slot : _slots)
            if (slot == f)
                return;

    for (Slot& slot : _slots)
        if (slot.empty())
        {
            slot = f;
            return;
        }

    _slots.push_back(f);
}

template<typename R, typename... Args>
void SimpleSignalBase<R, Args...>::disconnect(const Slot& f, bool all)
{
    std::size_t index = 0;

    { //Block for SpinLocker
        SpinLocker locker {_slotsLock}; (void) locker;
        for (std::size_t i = 0; i < _slots.size(); ++i)
            if (f == _slots[i])
            {
                _slots[i].reset();
                if (i == _slotIndex)
                    index = i;
                if (!all)
                    break;
            }
    }

    // Крутимся в холостом цикле  до тех  пор  пока  не  изменится  _slotIndex.
    // Такое поведение нужно на тот случай если объект вызывающий  disconnect()
    // делает это в момент своего  разрушения.  Если  дать  объекту  разрушится
    // преждевременно, то вызов текущего слота,  связанного  с  этим  объектом,
    // завершится SIGFAULT-том. Что бы этого избежать необходимо  приостановить
    // процесс разрушения объекта до тех пор пока обработчик слотов (метод call())
    // не перейдет к следующему слоту, и _slotIndex не поменяется
    while (true)
    {
        { //Block for SpinLocker
            SpinLocker locker {_slotsLock}; (void) locker;
            if (index == _slotIndex)
                continue;
        }
        break;
    }
}

template<typename R, typename... Args>
bool SimpleSignalBase<R, Args...>::exists(const Slot& f) const
{
    SpinLocker locker {_slotsLock}; (void) locker;
    for (const Slot& slot : _slots)
        if (slot == f)
            return true;

    return false;
}

template<typename R, typename... Args>
bool SimpleSignalBase<R, Args...>::empty() const
{
    SpinLocker locker {_slotsLock}; (void) locker;
    return _slots.empty();
}

template<typename R, typename... Args>
R SimpleSignalBase<R, Args...>::emit_(Args... args) const
{
    //std::lock_guard<std::mutex> locker {_emitLock}; (void) locker;

    // При передаче args... не используем вызов std::forward(),
    // см. пояснения в примечании к описанию модуля
    return call<R>(args...);
}

template<typename R, typename... Args>
void SimpleSignalBase<R, Args...>::clear()
{
    SpinLocker locker {_slotsLock}; (void) locker;
    _slots.clear();
}

template<typename R, typename... Args>
    template<typename Res>
Res SimpleSignalBase<R, Args...>::call(Args... args, typename disable_if_void<Res>::type) const
{
    Res res = {};
    if (_block) return res;

    Slot slot;
    ResetSlotIndex<decltype(_slotIndex)> resetIndex {&_slotIndex}; (void) resetIndex;

    _slotIndex = std::size_t(-1);
    while (true)
    {
        { //Block for SpinLocker
            SpinLocker locker {_slotsLock}; (void) locker;
            do {
                if (++_slotIndex >= _slots.size())
                    return res;

                slot = _slots[_slotIndex];
            }
            while (slot.empty());
        }
        // При передаче args... не используем вызов std::forward(),
        // см. пояснения в примечании к описанию модуля
        res = slot(args...);
    }
    return res;
}

template<typename R, typename... Args>
    template<typename Res>
void SimpleSignalBase<R, Args...>::call(Args... args, typename enable_if_void<Res>::type) const
{
    if (_block) return;

    Slot slot;
    ResetSlotIndex<decltype(_slotIndex)> resetIndex {&_slotIndex}; (void) resetIndex;

    _slotIndex = std::size_t(-1);
    while (true)
    {
        { //Block for SpinLocker
            SpinLocker locker {_slotsLock}; (void) locker;
            do {
                if (++_slotIndex >= _slots.size())
                    return;

                slot = _slots[_slotIndex];
            }
            while (slot.empty());
        }
        // При передаче args... не используем вызов std::forward(),
        // см. пояснения в примечании к описанию модуля
        slot(args...);
    }
}
