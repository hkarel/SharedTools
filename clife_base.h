/****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализованы базовый класс для управления жизнью объекта с использо-
  ванием внутреннего счетчика ссылок. Предполагается, что классы унаследованные
  от clife_base будут использоваться совместно со смарт-указателем clife_ptr.
****************************************************************************/

#pragma once
#include <atomic>


struct clife_base
{
    // Если предполагается, что основная работа с объектом clife_base будет
    // осуществляться через clife_ptr<>(), то в этом случае при создании
    // объекта clife_base предпочтительно счетчик жизни объекта выставлять в 0.
    // При работе через clife_ptr<>() счетчик жизни объекта будет увеличен
    // в конструкторе clife_ptr<>().
    // Если же при создании объекта clife_base не планируется его немедленная
    // передача во владение clife_ptr<>(), то целесообразно счетчик жизни выстав-
    // лять в 1. В этом случае в коде будет меньше вызовов метода add_ref().
    clife_base(bool add_ref = false) : clife_count(add_ref ? 1 : 0) {}
    virtual ~clife_base() {}

    inline void add_ref() const {++clife_count;}
    inline void release() const {if (--clife_count == 0) delete this;}

    mutable  std::atomic_long clife_count;
};

// /**
//   struct counter_life_base
// */
// struct counter_life_base
// {
// #if defined(USE_QT_INCREMENT)
//     mutable QAtomicInt counter_;
// #else
//     mutable volatile long counter_;
// #endif
//     // Счетчик выставленный в 0 предпочтительнее чем 1,
//     // в рабочем коде будет меньше вызовов функции add_ref().
//     counter_life_base() : counter_(0 /*1*/) {}
//     virtual ~counter_life_base() {}
// };
//
//
// /**
//   struct counter_life
// */
// /**
//   @brief counter_life - базовый класс для объектов использующих внутренний
//   счетчик ссылок.
// */
// template <typename T>
// struct counter_life : public counter_life_base
// {
//     counter_life() : counter_life_base() {}
//
// #if defined(USE_QT_INCREMENT)
//     inline void add_ref() const {counter_.ref();}
//     inline void release() const {if (counter_.deref() == 0) delete this;}
// #else
//     inline void add_ref() const {InterlockedIncrement(&counter_);}
//     inline void release() const {if (InterlockedDecrement(&counter_) == 0) delete this;}
// #endif
//
//     inline clife_ptr<T> self() const {
//         // Если изначальный счетчик жизни объекта равен 0, то add_ref() вызывать
//         // не нужно, это будет сделано автоматом в конструкторе clife_ptr<>().
//         //add_ref();
//         return clife_ptr<T>(static_cast<const T*>(this));
//     }
// };

