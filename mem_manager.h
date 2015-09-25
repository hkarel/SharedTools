/****************************************************************************
  Автор: Андрей Фролов (FAL), falinc@ukr.net
  http://www.gamedev.ru/code/articles/?id=4242

  Модифицировал: Карелин Павел (hkarel), hkarel@yandex.ru
****************************************************************************/

/**
  В модуле реализован менеджер памяти, основное назначение которго
  выделение памяти для большого количества маленьких однотипных
  объектов.

  Изменения по сравнению с оригинальной реализацией:
    - параметр N - длинна массива MemBlock переименован в m_arraySize и
      сделан членом класса MemManager, и задается в его конструкторе.
    - в структуре MemBlock упразднен union и поле T element. Дело в том,
      что если класс T имеет собственный конструктор по умолчанию, то
      использование union становится невозможным.
    - в структуре MemBlockArray изменен механизм выделения памяти
      для массива MemBlock (поле array_).
    - при распределении памяти для нового объекта добавлен вызов
      распределяющего оператора new - это нужно для объектов имеющих
      свои собственные конструкторы.
    - при возвращании памяти добавлен вызов деструктора объекта.
    - при освобождении занимаемой менеджером памяти рекурсивное удаление
      массивов MemBlockArray заменено на линейное (см. метод _clear()).
    - введена проверка на размер элемента T, это позволит ограничить
      использование элементов размер которых меньше размера указателей
      на них.
      Примечание: технически это ограничение можно легко обойти, но тогда
      эффективность использования выделенной памяти будет не так высока.
*/

#pragma once

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif

#include <new>
#include <atomic>
#include <stdlib.h>

#if defined(QT_CORE_LIB)
#include <QMutex>
#endif

#ifdef _MSC_VER
#include <windows.h>
#endif


/**
  @brief MemBlock, MemBlockArray - вспомогательные структуры.
*/
struct MemBlock
{
    MemBlock* next;
};

template <typename T> struct MemBlockArray
{
    MemBlockArray<T>* next;
    MemBlock* array_;
    MemBlockArray(int arraySize) :
        next(0),
        array_((MemBlock*) ::malloc(arraySize * sizeof(T)))
    {}
    ~MemBlockArray() {::free(array_);}
};


/**
  @brief Фиктивная стратегия блокировки потоков при создании/освобождении
  объектов. Используется только для однопоточных приложений.
*/
struct MemLockDummy
{
    void lock() {}
    void unlock() {}
};

/**
  @brief Стратегия блокировки потоков при создании/освобождении
  объектов на основе критических секций.
*/
#ifdef _MSC_VER
struct MemLockCS
{
    MemLockCS()   {::InitializeCriticalSection(&cs);}
    ~MemLockCS()  {::DeleteCriticalSection(&cs);}
    void lock()   {::EnterCriticalSection(&cs);}
    void unlock() {::LeaveCriticalSection(&cs);}
    CRITICAL_SECTION cs;
};
#endif

/**
  @brief Стратегия блокировки потоков при создании/освобождении
  объектов на основе механизмов блокировки Qt.
*/
#ifdef QT_CORE_LIB
struct MemLockQt
{
    MemLockQt() : mutex(QMutex::Recursive) {}
    void lock() {mutex.lock();}
    void unlock() {mutex.unlock();}
    QMutex mutex;
};

struct MemLockSpinQt // spin lock
{
    MemLockSpinQt() {}
    void lock()   {while (!atomic.testAndSetAcquire(0x0, 0x1));}
    void unlock() {while (!atomic.fetchAndStoreRelease(0x0));}
    QAtomicInt atomic;
};
#endif // QT_CORE_LIB

/**
  @brief Стратегия блокировки потоков при создании/освобождении
  объектов на основе механизмов атомарных блокировок STL.
  TODO: Механизм должен быть рекурсивным, либо флаг FreeArrays должен
        быть выставлен в FALSE. Нужно доработать.
*/
struct MemLockSpin // spin lock
{
    MemLockSpin() {}
    void lock()   NOEXCEPT {while (atomic.test_and_set(std::memory_order_acquire));}
    void unlock() NOEXCEPT {atomic.clear(std::memory_order_release);}
    std::atomic_flag atomic = ATOMIC_FLAG_INIT;
};

/**
  @brief Вспомогательный класс, используется для блокировки потоков.
*/
template<typename T> struct MemLocker
{
    MemLocker(T* l) : locker(l) {locker->lock();}
    ~MemLocker() {locker->unlock();}
    T* locker;
};


/**
  @brief Класс MemManager.
*/
template <
    typename T,
    typename LockT = MemLockDummy
    //int      ArraySize  = 1000,
    //bool     FreeArrays = true
>
class MemManager
{
public:
    /// @brief Конструктор
    ///
    /// @param[in] arraySize Определяет количество элементов MemBlock в массиве MemBlockArray.
    /// @param[in] freeArrays Определяет необходимость разрушать объекты MemBlockArray.
    //MemManager(int arraySize = ArraySize, bool freeArrays = FreeArrays) :
    MemManager(int arraySize = 128, bool freeArrays = true) :
        _freeBlocks(0),
        _arrays(0),
        _arraySize(arraySize),
        _freeArrays(freeArrays),
        _callsCount(0)
    {
        // Если в этом месте получили ошибку компиляции - это означает, что размер
        // элемента меньше размера его указателя. Для данного менеджера памяти эта
        // ситуация недопустима.
        static_assert((sizeof(T) >= sizeof(T*)), "Element size should be larger than the size of its pointer");
    }
    //~MemManager()
    //{
    //    if (m_freeArrays) _clear();
    //}
    ~MemManager()
    {
        //clear();

        // При разрушении менеджера выделенная им память должна быть освобождена
        // в любом случае, иначе мы получаем ситуацию с утечкой. Тем не менее
        // вызов метода clear() не всегда приводит к освобождению выделенной
        // памяти. Метод clear() освободит выделенные ресурсы только в том случае
        // когда счетчик _callsCount будет равен нулю. Чтобы это условие наступи-
        // ло - для каждого созданного объекта должен быть вызван метод free().
        // Однако бывают ситуации, когда метод free(), с точки зрения экономии
        // вычислительных ресурсов, нецелесообразно вызывать для каждого удаляе-
        // мого объекта. Например рассмотрим список из объектов POD-типа, для
        // которых деструктор вызывать необязательно. Так же этот список работает
        // в режиме только на вставку, а удаление объектов происходит только
        // в момент разрушения списка. В этой ситуации можно пропустить вызов free()
        // и таким образом сэкономить на перерасчете указателей в менеджере памяти
        // (когда речь идет о миллионных списках - это может дать существенную
        // экономию).
        MemLocker<LockT> locker(&_lock); (void) locker;
        _clear();
    }

    T* allocate()
    {
        MemLocker<LockT> locker(&_lock); (void) locker;
        grow();
        MemBlock* b = _freeBlocks;
        _freeBlocks = _freeBlocks->next;
        ++_callsCount;
        return reinterpret_cast<T*>(b);
    }

    void free(T* t)
    {
        if (t) {
            MemLocker<LockT> locker(&_lock); (void) locker;
            MemBlock* b = reinterpret_cast<MemBlock*>(t);
            b->next = _freeBlocks;
            _freeBlocks = b;
            --_callsCount;
            if (_freeArrays && (_callsCount == 0))
                _clear();
        }
    }

    T* create()
    {
        T* p = this->allocate();
        new (p) T();
        return p;
    }

    T* create(const T& t)
    {
        T* p = this->allocate();
        new (p) T(t);
        return p;
    }

    void destroy(T* t)
    {
        if (t) {
            t->~T();
            this->free(t);
        }
    }

    void clear()
    {
        MemLocker<LockT> locker(&_lock); (void) locker;
        if (_callsCount == 0)
            _clear();
    }

public:
//    // Копирующий конструктор и оператор присваивания делаем фиктивными,
//    // это позволит предотвратить копирование экземпляра данного класса и
//    // в тоже время позволит использовать в структурах где соответствующие
//    // копирующий конструктор и оператор присваивания генерируются
//    // компилятором по умолчанию.
//    MemManager(const MemManager&)
//    {
//        break_point
//    }
//    MemManager& operator= (const MemManager&)
//    {
//        break_point
//        return *this;
//    }

private:
    // Запрет конструктора по умолчанию.
    MemManager() = delete;
    MemManager(MemManager&&) = delete;
    MemManager(const MemManager&) = delete;
    MemManager& operator= (MemManager&&) = delete;
    MemManager& operator= (const MemManager&) = delete;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"

#ifndef NDEBUG
// Метод _clear() публично доступен только для отладки
public:
#else
private:
#endif
    void _clear()
    {
        if (_arrays) {
            MemBlockArray<T>* a;
            while (a = _arrays->next) {
                delete _arrays;
                _arrays = a;
            }
            delete _arrays;
        }
        _freeBlocks = 0;
        _arrays = 0;
    }

#pragma GCC diagnostic pop

private:
    void grow()
    {
        if (_freeBlocks == 0) {
            MemBlockArray<T>* a = new MemBlockArray<T>(_arraySize);
            a->next = _arrays;
            _arrays = a;
            union {MemBlock* arr; char* offset;};
            arr = _arrays->array_;
            //int ii;
            for (int i = 0; i < _arraySize; ++i) {
                arr->next = _freeBlocks;
                _freeBlocks = arr;
                //MemBlock* aa = arr;
                offset += sizeof(T);
                //ii = unsigned int((char*)arr) - unsigned int((char*)aa);
            }
        }
    }

private:
    MemBlock*         _freeBlocks;
    MemBlockArray<T>* _arrays;
    int               _arraySize;
    bool              _freeArrays;
    unsigned int      _callsCount;
    LockT             _lock;
};

