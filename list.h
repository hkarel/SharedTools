/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2005 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  В модуле реализован класс-список (List) с доступом к элементам по индексу.

  Реализованы механизмы:
    - быстрой сортировки;
    - частичной сортировки;
    - быстрого поиска;
    - грубого поиска;
    - возможность добавлять элементы в список без нарушения порядка
      сортировки
*****************************************************************************/

#pragma once

#include <exception>
#include <string.h>
#include "break_point.h"

namespace lst {

/// @brief Определяет является ли список владельцем (контейнером)
///        для элементов
enum class Container {No = 0, Yes = 1};

/// @brief Определяет необходимость сжатия списка после удаления
///        из него элемента
enum class CompressList {No = 0, Yes = 1};

/// @brief Признак поиска простым перебором (метод грубой силы)
enum class BruteForce {No = 0, Yes = 1};

/// @brief Флаги направления сортировки
enum class SortMode
{
  Down = 0,  /// Сортировать по убыванию
  Up   = 1   /// Сортировать по возрастанию
};

/// @brief Флаги сдвига списка
enum class ShiftMode
{
  Left  = 0, /// Сдвинуть элементы списка влево
  Right = 1  /// Сдвинуть элементы списка вправо
};

/// @brief Флаги состояний сортировки
enum class SortState
{
  Unknown    = 0,  /// Список находится в не отсортированном состоянии
  Up         = 1,  /// Список отсортирован по возрастанию
  Down       = 2,  /// Список отсортирован по убыванию
  CustomUp   = 3,  /// Список частично отсортирован по возрастанию
  CustomDown = 4   /// Список частично отсортирован по убыванию
};

class FindResult;

template<typename ListT, typename CompareT>
FindResult find(const ListT&, const CompareT&);

template<typename ListT, typename CompareL>
FindResult firstFindResultL(const ListT&, const CompareL&, const FindResult&);

template<typename ListT, typename CompareL>
FindResult lastFindResultL(const ListT&, const CompareL&, const FindResult&);

template<typename, typename, typename> class CustomList;
template<typename, typename, typename> class List;

/// @brief Проверяет принадлежность значения index диапазону [minVal-maxVal)
///
/// Значение minVal включено в диапазон проверки, maxVal - нет
template<typename T>
inline bool inRange(T index, T minVal, T maxVal)
{
  return ((index >= minVal) && (index < maxVal));
}

/// @brief Функция проверяет находится ли значение index в границах списка
///
/// Если index удовлетворяет условию 0 <= index < count() функция вернет TRUE
template<typename ListType>
inline bool checkBounds(int index, const ListType& list)
{
  return inRange(index, 0, list.size());
}

/**
  @brief Класс ListExcept. Используется для обработки исключений
*/
class ListExcept : public std::exception
{
  char _msg[512];
public:
  ListExcept(const char* msg, const char*  func)
  {
    memset(_msg, 0, sizeof(_msg));
    strcat(_msg, "Function [");
    strcat(_msg, func);
    strcat(_msg, "]: ");
    strcat(_msg, msg);
  }
  virtual const char* what() const noexcept {return _msg;}
};

constexpr const char* ERR_NOCREATE_OBJ =
  "Impossible create object of the class. (Container::No)";

#if defined(_MSC_VER)
#define LIST_EXCEPT(MSG) ListExcept(MSG, __FUNCTION__)
#else
#define LIST_EXCEPT(MSG) ListExcept(MSG, __func__)
#endif

#ifndef NDEBUG
#  define CHECK_BORDERS(INDEX) \
     if ((INDEX < 0) || (INDEX >= d->count)) throw LIST_EXCEPT("Index out of bounds");

#  define CHECK_NOTLESS(INDEX, LESS) \
     if (INDEX < LESS) throw LIST_EXCEPT("Index must not be less than "#LESS);

#  define CHECK_INTERNAL_DATA_PTR(DPTR) \
     if (DPTR == nullptr) throw LIST_EXCEPT("Internal data-pointer is null");
#else
#  define CHECK_BORDERS(INDEX)
#  define CHECK_NOTLESS(INDEX, LESS)
#  define CHECK_INTERNAL_DATA_PTR(DPTR)
#endif // NDEBUG

/**
  @brief Класс BreakCompare. Используется в функциях/стратегиях сравнения
  элементов для прерырания процессов сортировки или поиска
*/
struct BreakCompare {};

/**
  @brief Класс FindResult - результат функций поиска.
  Для отсортированного списка:
    - если элемент найден, то success() == true,  а index() вернет позицию эле-
      мента в списке;
    - если элемент не найден, то success() == false, а index()  вернет  позицию
      в которую можно вставить ненайденный элемент, и список при этом останется
      отсортированным.
      Для  вставки  элемента в отсортированный список используется функция
      List::addInSort().
  Для неотсортированного списка:
    - если элемент найден, то  success() == true, а функция index() вернет
      позицию элемента в списке;
    - если  элемент  не  найден,  то success() == false, а функция index()
      вернет количество элементов в списке (count)
*/
class FindResult
{
public:
  FindResult() : _index(-1), _success(false), _bruteForce(false) {}
  bool success()    const noexcept {return  _success;}
  bool failed()     const noexcept {return !_success;}
  int  index()      const noexcept {return _index;}
  bool bruteForce() const noexcept {return _bruteForce;}

  explicit operator bool() const noexcept {return _success;}

private:
  int      _index;
  unsigned _success    : 1;
  unsigned _bruteForce : 1;
  unsigned _reserved   : 30;

  FindResult(bool success, BruteForce bruteForce, int index)
    : _index(index), _success(success), _bruteForce(bool(bruteForce))
  {}
  template<typename, typename, typename> friend class CustomList;

  template<typename ListT, typename CompareT>
  friend FindResult find(const ListT&, const CompareT&);

  template<typename ListT, typename CompareL>
  friend FindResult firstFindResultL(const ListT&, const CompareL&, const FindResult&);

  template<typename ListT, typename CompareL>
  friend FindResult lastFindResultL(const ListT&, const CompareL&, const FindResult&);
};

struct FindResultRange
{
  FindResult first;
  FindResult last;
};

/**
  @brief Сервисная структура, используется для агрегации расширенных параметров
         поиска
*/
struct FindExtParams
{
  /// Признак поиска простым перебором (метод грубой силы).
  /// Если bruteForce = Yes поиск будет происходить простым перебором, даже для
  /// отсортированного списка
  BruteForce bruteForce = {BruteForce::No};

  /// Индекс с которого начинается поиск.
  /// Если startFindIndex >= list::count(),  то функция поиска вернет результат
  /// со статусом  FindResult::failed() = TRUE.  При этом если элемент с данным
  /// статусом будет добавлен в отсортированный список через функцию addInSort(),
  /// то флаг сортировки будет сброшен (см. пояснения к функции list::addInSort())
  int startFindIndex = {0};

  FindExtParams() = default;
  FindExtParams(BruteForce bruteForce)
    : bruteForce(bruteForce), startFindIndex(0)
  {}
};

/**
  @brief Сервисная структура, используется для агрегации расширенных параметров
         сортировки
*/
struct SortExtParams
{
  /// loSortBorder, hiSortBorder  Границы  сортировки,  позволяют  производить
  /// сортировку по указанному диапазону. При назначении диапазона соблюдаются
  /// следующие требования:
  /// 1) 0 <= loSortBorder < count(), в противном случае loSortBorder
  ///    выставляется в 0.
  /// 2) loSortBorder < hiSortBorder <= count(), в противном случае
  ///    hiSortBorder выставляется в count (количество элементов в списке).
  /// Значения по умолчанию {0, -1} предполагают сортировку по всему диапазону
  int loSortBorder = {0};
  int hiSortBorder = {-1};

  SortExtParams() = default;
  SortExtParams(int loSortBorder, int hiSortBorder = -1)
    : loSortBorder(loSortBorder), hiSortBorder(hiSortBorder)
  {}
};

/**
  @brief Функции выполняют поиск перебором (грубый поиск).

  Первые  две  функции  выполнены  для  использования  с  lambda-функциями
  с сигнатурой:
    int [](const ListT::ValueType* item), где item элемент списка.

  Следующие три функции  в качестве  compare-элемента  используют  функцию
  или функтор со следующей сигнатурой:
    int function(const T* item1, const ListT::ValueType* item2)

  Примечание: в качестве контейнера ListT можно использовать std::vector
*/
template<typename ListT, typename CompareL>
FindResult find(const ListT& list, const CompareL& compare);

template<typename ListT, typename CompareL>
auto findItem(const ListT& list, const CompareL& compare) -> typename ListT::pointer;

//---
template<typename T, typename ListT, typename CompareT>
FindResult find(const T* item, const ListT& list, const CompareT& compare);

template<typename T, typename ListT, typename CompareT>
FindResult findRef(const T& item, const ListT& list, const CompareT& compare);

template<typename T, typename ListT, typename CompareT>
T* findItem(const T* item, const ListT& list, const CompareT& compare);

/**
  @brief Группа функций выполняет поиск первого или последнего элемента в после-
         довательности одинаковых значений.

  Если список содержит не уникальные значения, то при его сортировке одинаковые
  значения будут идти друг за другом. При использовании функций быстрого поиска
  в таком  списке,  с  высокой  долей  вероятности,  найденное  значение  будет
  не первым, и не последним в последовательности  одинаковых  значений.  Однако,
  наиболее часто требуются первое или последнее значения.
  В качестве результата возвращается индекс первого/последнего элемента в после-
  довательности
*/

/// Примечание: в качестве compare-элемента используется lambda-функция
/// с сигнатурой:
/// int [](const ListT::ValueType* item), где item элемент списка
template<typename ListT, typename CompareL>
FindResult firstFindResultL(const ListT& list, const CompareL& compare,
                            const FindResult& fr);

template<typename ListT, typename CompareL>
FindResult lastFindResultL(const ListT& list, const CompareL& compare,
                           const FindResult& fr);

/// Примечание: в качестве compare-элемента используется функция или функтор
/// с сигнатурой:
/// int function(const ListT::ValueType* item1, const ListT::ValueType* item2)
template<typename ListT, typename CompareT>
FindResult firstFindResult(const ListT& list, const CompareT& compare,
                           const FindResult& fr);

template<typename ListT, typename CompareT>
FindResult lastFindResult(const ListT& list, const CompareT& compare,
                          const FindResult& fr);

/**
  @brief Группа функций выполняет поиск первого и последнего элемента в после-
         довательности одинаковых значений
*/
template<typename ListT, typename CompareL>
FindResultRange rangeFindResultL(const ListT& list, const CompareL& compare,
                                 const FindResult& fr);

template<typename ListT, typename CompareT>
FindResultRange rangeFindResult(const ListT& list, const CompareT& compare,
                                const FindResult& fr);

/**
  @brief Макрос используется в классе-стратегии сортировки и поиска
*/
#define LIST_COMPARE_ITEM(ITEM1, ITEM2) \
  ((ITEM1 > ITEM2) ? 1 : ((ITEM1 < ITEM2) ? -1 : 0))

/**
  @brief Макрос  используется  в  классе-стратегии  сортировки и поиска в тех
         случаях,  когда сортировка или поиск выполняются по нескольким полям.
         Ниже приведен пример  использования.  Здесь  сортировка  выполняется
         по трем полям с убывающим приоритетом сравнения от field1 к field3.
         struct Compare
         {
           int operator() (const Type* item1, const Type* item2) const
           {
             LIST_COMPARE_MULTI_ITEM( item1->field1, item2->field1)
             LIST_COMPARE_MULTI_ITEM( item1->field2, item2->field2)
             return LIST_COMPARE_ITEM(item1->field3, item2->field3);
           }
         };

         Альтернативный вариант записи
         struct Compare
         {
           int operator() (const Type* item1, const Type* item2) const
           {
             LIST_COMPARE_MULTI_ITEM(item1->field1, item2->field1)
             LIST_COMPARE_MULTI_ITEM(item1->field2, item2->field2)
             LIST_COMPARE_MULTI_ITEM(item1->field3, item2->field3)
             return 0;
           }
         };
*/
#define LIST_COMPARE_MULTI_ITEM(ITEM1, ITEM2) \
  if (ITEM1 != ITEM2) return (ITEM1 < ITEM2) ? -1 : 1;

/**
  @brief Класс-стратегия используется для сортировки и поиска.
*/
template<typename T> struct CompareItem
{
  /// @param[in] item1 Первый сравниваемый элемент.
  /// @param[in] item2 Второй сравниваемый элемент.
  /// @return Результат сравнения.
  /// Примечание: Оператор не должен быть виртуальным.  Если оператор сделать
  /// виртуальным, то для каждого инстанциируемого класса придется определять
  /// операторы "<", ">", "==", что сводит "на нет" идею класса-стратегии
  /// применительно к сортировке
  int operator() (const T* item1, const T* item2) const
  {
    return LIST_COMPARE_ITEM(*item1, *item2);
  }
};

/**
  @brief Фиктивный класс-стратегия используется в тех случаях,  когда  нужно
  явно указать, что никакие стратегии сортировки использовать не планируется
*/
struct CompareItemDummy {};

/**
  @brief Распределитель памяти для элементов списка
*/
template<typename T> struct AllocItem
{
  /// @brief Функция создания объектов.
  ///
  /// Примечания:
  /// 1. Должно быть  две  функции  create().  Если  использовать  только  одну
  ///    функцию вида:
  ///      T* create(const T* x = 0) {return (x) ? new T(*x) : new T();}
  ///    то  компилятор  будет  требовать   обязательное  наличие  конструктора
  ///    копирования у инстанциируемого класса.
  /// 2. Функции  create()/destroy()  не  могут  быть  статическими,  так  как
  ///    аллокатор может иметь состояния.
  /// 3. Функции create()/destroy() должны быть  неконстантными,  так  как  их
  ///    вызов в конкретных реализациях может приводить к изменению  состояния
  ///    экземпляра распределителя памяти
  T* create() {return new T();}
  T* create(const T* x) {return (x) ? new T(*x) : new T();}

  /// @brief Функция разрушения элементов
  void destroy(T* x) {
    static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
    delete x;
  }
};

/**
  @brief Класс CustomList.

  Класс не является потокобезопасным, следует с осторожностью допускать
  одновременный вызов методов данного класса из разных потоков
*/
template<
  typename T,
  typename Compare,
  typename Allocator
>
class CustomList
{
public:
  typedef T*         PointerType;
  typedef T          ValueType;
  typedef Compare    CompareType;
  typedef Allocator  AllocatorType;

  typedef CustomList<T, Compare, Allocator>  CustomListType;

  // Для совместимости с STL
  typedef       T*  pointer;
  typedef const T*  const_pointer;
  typedef       T&  reference;
  typedef const T&  const_reference;
  typedef       T** iterator;
  typedef const T** const_iterator;
  typedef       T   value_type;

public:
  /// @brief Функция поиска по адресу элемента.
  ///
  /// @param[in]  item  Искомый элемент.
  /// @return Индекс искомого элемента, в случае неудачи возвращает -1
  int indexOf(const T* item) const;

  /// @brief Функция поиска по адресу элемента.
  ///
  /// @param[in]  item  Искомый элемент.
  /// @param[out] index В случае удачи возвращает индекс искомого элемента,
  /// @return В случае удачного поиска возвращает TRUE
  bool indexOf2(const T* item, int& index) const;

  /// @brief Функция проверяет находится ли значение index в границах списка.
  ///
  /// Если index удовлетворяет условию 0 <= index < count(), то функция
  /// возвращает TRUE, в противном случае функция возвращает FALSE
  bool checkBounds(int index) const {return inRange(index, 0, d->count);}

  /// @brief Функции поиска.
  ///
  /// @param[in] item Искомый элемент.
  /// @param[in] extParams Используется для задания расширенных параметров
  ///            поиска.
  /// @return Структура с результатом поиска
  template<typename U>
  FindResult find(const U* item,
                  const FindExtParams& extParams = FindExtParams()) const;

  /// @brief Перегруженные функции поиска, определена для удобства использования.
  ///
  /// @return Возвращает указатель на искомый элемент, если элемент не найден -
  /// функция вернет nullptr
  template<typename U>
  T* findItem(const U* item,
              const FindExtParams& extParams = FindExtParams()) const;

  /// @brief Перегруженные функции поиска, определена для удобства использования.
  ///
  /// @param[in] item Искомый элемент передаваемый по ссылке
  template<typename U>
  FindResult findRef(const U& item,
                     const FindExtParams& extParams = FindExtParams()) const;

  /// @brief Перегруженные функции поиска, определены для удобства использования.
  ///
  /// Позволяет выполнять поиск со стратегией поиска отличной от той,
  /// что была определена в классе-контейнере
  template<typename U, typename CompareU>
  FindResult find(const U* item, const CompareU& compare,
                  const FindExtParams& extParams = FindExtParams()) const;

  template<typename U, typename CompareU>
  T* findItem(const U* item, const CompareU& compare,
              const FindExtParams& extParams = FindExtParams()) const;

  template<typename U, typename CompareU>
  FindResult findRef(const U& item, const CompareU& compare,
                     const FindExtParams& extParams = FindExtParams()) const;

  /// @brief Функции поиска.
  ///
  /// В качестве стратегии поиска используется lambda-функция с сигнатурой
  /// int [](const CustomList::ValueType* item)
  template<typename CompareL>
  FindResult findL(const CompareL& compare,
                   const FindExtParams& extParams = FindExtParams()) const;

  template<typename CompareL>
  T* findItemL(const CompareL& compare,
               const FindExtParams& extParams = FindExtParams()) const;

  /// @brief Доступ к элементу списка по индексу.
  ///
  /// @return Возвращает указатель на элемент
  T* item(int index) const {CHECK_BORDERS(index); return d_func()->list[index];}

  /// @brief Доступ к элементу списка по индексу.
  ///
  /// @return Возвращает ссылку на элемент
  T&       itemRef(int index) {return *item(index);}
  const T& itemRef(int index) const {return *item(index);}

  /// @brief Доступ к элементу списка по индексу.
  ///
  /// @return Возвращает ссылку на элемент, аналогично функции itemRef()
  T&       operator[] (int index) {return *item(index);}
  const T& operator[] (int index) const {return *item(index);}

  /// @brief Доступ к элементу списка по индексу.
  ///
  /// @return Возвращает ссылку на элемент. Функция обеспечивает совместимость
  /// с STL
  const T& at(int index) const {return *item(index);}

  /// @brief Возвращает указатель на адрес первого элемента в линейном массиве
  /// указателей на элементы
  T** listBegin() const {return d_func()->list;}

  /// @brief Возвращает указатель на адрес идущий за последним элементом
  /// в линейном массиве указателей на элементы
  T** listEnd() const {return (d_func()->list + d_func()->count);}

  /// @brief Возвращает указатель на адрес первого элемента в линейном массиве
  /// указателей на элементы.
  /// Используется в конструкции вида: for (T* t : List)
  T** begin() const {return listBegin();}

  /// @brief Возвращает указатель на адрес идущий за последним элементом
  /// в линейном массиве указателей на элементы.
  /// Используется в конструкции вида: for (T* t : List)
  T** end() const {return listEnd();}

  /// @brief Возвращает количество элементов в списке
  int count() const {return d_func()->count;}

  /// @brief Возвращает зарезервированную длинну массива list()
  int capacity() const {return d_func()->capacity;}

  /// @brief Признак того, что список является контейнером.
  ///
  /// Если container() == Container::Yes, то при разрушении или очистке списка
  /// все элементы будут автоматически разрушены, в противном случае  элементы
  /// списка разрушены не будут
  Container container() const {return d_func()->container;}

  /// @brief Определяет состояние сортировки
  SortState sortState() const {return d_func()->sortState;}

  /// @brief Возвращает ссылку на стратегию сортировки
  Compare& compare() {return d_func()->compare;}

  /// @brief Возвращает константную ссылку на стратегию сортировки
  const Compare& compare() const {return d_func()->compare;}

  /// @brief Возвращает ссылку на распределитель памяти
  Allocator& allocator() {return d_func()->allocator;}

  /// @brief Возвращает константную ссылку на распределитель памяти
  const Allocator& allocator() const {return d_func()->allocator;}

  /// @brief Возвращает количество элементов в списке. Функция обеспечивает
  /// совместимость с STL
  int size() const {return count();}

  /// @brief Возвращает true если список пуст
  bool empty() const {return (count() == 0);}

  /// @brief Возвращает первый элемент в списке.
  /// Если список пустой, то возвращает нуль
  T* first() const {return (d_func()->count) ? d->list[0] : nullptr;}

  /// @brief Возвращает последний элемент в списке.
  /// Если список пустой, то возвращает нуль
  T* last() const {return (d_func()->count) ? d->list[d->count - 1] : nullptr;}

  template<typename IteratorT> class Range
  {
  public:
    IteratorT** begin() const {return _begin;}
    IteratorT** end()   const {return _end;}
  private:
    Range() {}
    IteratorT** _begin;
    IteratorT** _end;
    template<typename, typename, typename> friend class CustomList;
  };
  typedef Range<T> RangeType;

  /// @brief Сервисные функции, возвращают структуру Range содержащую пару
  ///        итераторов указывающих на первый элемент и на элемент  идущий
  ///        за последним для заданного диапазона значений.
  ///
  /// Основное назначение этих функций - это использование в конструкции
  /// вида: for (T* t : range).
  /// @param[in] index1 Индекс первого элемента в диапазоне.
  /// @param[in] index2 Индекс последнего элемента в диапазоне.
  /// @return Структура Range определяющая диапазон
  RangeType range(int index1, int index2) const;
  RangeType range(const FindResultRange&) const;

private:
  CustomList() {}
  ~CustomList() {}

private:
  CustomList(CustomListType&&) = delete;
  CustomList(const CustomListType&) = delete;

  CustomListType& operator= (CustomListType&&) = delete;
  CustomListType& operator= (const CustomListType&) = delete;

private:
  template<typename DataT> struct Data
  {
    DataT**   list      = {nullptr};
    int       count     = {0};
    int       capacity  = {0};
    SortState sortState = {SortState::Unknown};
    Container container = {Container::Yes};
    Compare   compare;
    Allocator allocator;
  };
  typedef Data<T> DataType;
  DataType* d = {nullptr};

  DataType* d_func() const {CHECK_INTERNAL_DATA_PTR(d); return d;}
  template<typename, typename, typename> friend class List;
};

/**
  @brief Класс List.

  Класс не является потокобезопасным
*/
template<
  typename T,
  typename Compare = CompareItem<T>,
  typename Allocator = AllocItem<T>
>
class List : public CustomList<T, Compare, Allocator>
{
public:
  typedef List<T, Compare, Allocator>               SelfListType;
  typedef CustomList<T, Compare, Allocator>         CustomListType;

  typedef typename CustomListType::PointerType      PointerType;
  typedef typename CustomListType::ValueType        ValueType;
  typedef typename CustomListType::CompareType      CompareType;
  typedef typename CustomListType::AllocatorType    AllocatorType;
  typedef typename CustomListType::RangeType        RangeType;

  // Для совместимости с STL
  typedef typename CustomListType::pointer          pointer;
  typedef typename CustomListType::const_pointer    const_pointer;
  typedef typename CustomListType::reference        reference;
  typedef typename CustomListType::const_reference  const_reference;
  typedef typename CustomListType::iterator         iterator;
  typedef typename CustomListType::const_iterator   const_iterator;
  typedef typename CustomListType::value_type       value_type;

public:
  explicit List(Container container = Container::Yes);
  explicit List(const Allocator&, Container container = Container::Yes);

  List(CustomListType&&);
  List(SelfListType&&);

  ~List();

  List(const SelfListType&) = delete;
  SelfListType& operator= (SelfListType&&) = delete;
  SelfListType& operator= (const SelfListType&) = delete;

  /// @brief Добавляет новый элемент T в конец списка.
  ///
  /// Класс элемента должен иметь конструктор по умолчанию.
  /// @return Возвращает указатель на добавленный элемент
  T* add();

  /// @brief Добавляет уже созданный элемент T в конец списка.
  ///
  /// @param[in] item Добавляемый элемент.
  /// @return Возвращает указатель на добавленный элемент
  T* add(T* item);

  /// @brief Добавляет копию элемента T в конец списка.
  /// @return Возвращает указатель на добавленный элемент
  T* addCopy(const T& item);

  /// @brief Добавляет новый элемент T в указанную позицию в списке.
  ///
  /// Если index больше количества элементов в списке (count()),  то  элемент
  /// будет добавлен в конец списка. Класс элемента T должен иметь конструктор
  /// по умолчанию.
  /// @param[in] index Позиция вставки нового элемента.
  /// @return Возвращает указатель на добавленный элемент
  T* insert(int index = 0);

  /// @brief Добавляет существующий элемент T в указанную позицию в списке.
  ///
  /// Если index больше количества элементов в списке (count()),  то  элемент
  /// будет добавлен в конец списка. Класс элемента T должен иметь конструктор
  /// по умолчанию.
  /// @param[in] item Добавляемый элемент.
  /// @param[in] index Позиция вставки нового элемента.
  /// @return Возвращает указатель на добавленный элемент
  T* insert(T* item, int index = 0);

  /// @brief Добавляет копию элемента T в указанную позицию в списке.
  T* insertCopy(const T& item, int index = 0);

  /// @brief Удаляет элемент из списка.
  ///
  /// При удалении элемента из списка происходит его разрушение.
  /// @param[in] index Индекс удаляемого элемента.
  /// @param[in] compressList Признак сжатия списка. Если compressList
  ///     равен CompressList::No, то ячейкам в массиве списка присваивается 0.
  ///     Таким образом получается разряженный список содержащий null-значения
  void remove(int index, CompressList compressList = CompressList::Yes);

  /// @brief Удаляет элемент из списка.
  ///
  /// При удалении элемента из списка происходит его разрушение.
  /// @param[in] item Элемент удаляемый из списка.
  /// @param[in] compressList Признак сжатия списка.
  /// @return Индекс удаленного элемента, если элемент не присутствовал
  ///         в списке возвращает int(-1)
  int removeItem(T* item, CompressList compressList = CompressList::Yes);

  /// @brief Удаляет последний элемент из списка.
  ///
  /// При удалении элемента из списка происходит его разрушение
  void removeLast();

  /// @brief Удаляет элементы из списка согласно условию condition.
  ///
  /// При удалении элементов из списка происходит их разрушение.
  /// @param[in] condition Функор или функция с сигнатурой bool condition(T*).
  ///            Если condition возвращает TRUE, то элемент удаляется из списка.
  /// @param[in] compressList Признак сжатия списка
  template<typename Condition>
  void removeCond(const Condition& condition,
                  CompressList compressList = CompressList::Yes);

  /// @brief Удаляет элементы из списка.
  ///
  /// При удалении элементов из списка происходит их разрушение.
  /// @param[in] index Индекс с которого начнется удаление элементов.
  /// @param[in] count Количество удаляемых элементов.
  /// @param[in] compressList Признак сжатия списка
  void removes(int index, int count,
               CompressList compressList = CompressList::Yes);

  /// @brief Заменяет элемент в списке.
  ///
  /// @param[in] index Индекс в котором будет произведена замена.
  /// @param[in] item Новый элемент в списке.
  /// @param[in] keepSortState Признак сохранения флага сортировки
  void replace(int index, T* item, bool keepSortState = false);

  /// @brief Удаляет элемент из списка, при этом разрушения элемента не происходит.
  ///
  /// @param[in] index Индекс удаляемого элемента.
  /// @param[in] compressList Признак сжатия списка (см. описание для remove()).
  /// @return Возвращает указатель на удаленный из списка элемент
  T* release(int index, CompressList compressList = CompressList::Yes);

  /// @brief Удаляет элемент из списка, при этом разрушения элемента не происходит.
  ///
  /// @param[in] item Удаляемый элемент.
  /// @param[in] compressList Признак сжатия списка (см. описание для remove()).
  /// @return В случае успешного удаления возвращает индекс удаленного элемента
  int releaseItem(T* item, CompressList compressList = CompressList::Yes);

  /// @brief Удаляет последний элемент из списка при этом разрушения элемента
  /// не происходит.
  ///
  /// Типичное использование этого оператора в конструкциях вида:
  /// while (T* t = list.releaseLast()) {
  ///   ...
  ///   delete t;
  /// }
  /// Такой подход может иметь преимущества по скорости при условии большого
  /// количества элементов в списке перед конструкцией вида:
  /// while (list.count()) {
  ///   T* t = list.release(0);
  ///   ...
  ///   delete t;
  /// }
  /// Если в списке нет элементов будет возвращен nullptr
  T* releaseLast();

  /// @brief Меняет местами элементы в списке.
  ///
  /// @param[in] index1 Индекс первого элемента.
  /// @param[in] index2 Индекс второго элемента
  void exchange(int index1, int index2);

  /// @brief Перемещает элемент из позиции curIndex в позицию newIndex
  void move(int curIndex, int newIndex);

  /// @brief Очищает список элементов
  void clear();

  /// @brief Устанавливает новый размер массива указателей на элементы списка.
  ///
  /// Указатель на массив указателей элементов списка можно получить через
  /// функцию listBegin().
  /// @param[in] newCapacity Новая длинна массива указателей
  void setCapacity(int newCapacity);

  /// @brief Изменяет глобальную стратегию сортировки и поиска.
  void setCompare(const Compare& val) {d_func()->compare = val;}

  /// @brief Функция сжатия списка.
  ///
  /// Удаляет нулевые ячейки, которые могли образоваться в массиве указателей
  /// на элементы при вызове методов удаления элементов (remove(), release())
  /// с параметром compressList равным CompressList::No
  void compressList();

  /// @brief Функция добавляет элемент в отсортированный список
  ///        (флаг сортировки не сбрасывается).
  ///
  /// Предполагается, что вставка нового элемента не нарушит порядок сортировки
  /// элементов в отсортированном списке. Чтобы  порядок  сортировки  оставался
  /// неизменным - индекс вставки должен  быть  корректным.  Корректный  индекс
  /// можно определить при помощи функции find() и структуры FindResult.
  /// Если элемент не найден в списке, то в структуре FindResult будет возвращен
  /// индекс для корректной вставки элемента в список.
  /// Если флаг сортировки не определен  (список не отсортирован),  то  элемент
  /// будет добавлен в конец списка.  При частичной сортировке - флаг состояния
  /// сортировки будет сброшен, а элемент будет добавлен в конец списка.
  /// ПРИМЕЧАНИЕ: В случае, когда перед вставкой  элемента  производится  поиск
  /// с флагом  bruteForce = TRUE, и результат поиска  FindResult  используется
  /// для вставки нового элемента через  addInSort() - элемент  будет  добавлен
  /// в конец списка, а флаг состояния сортировки будет сброшен.
  /// @param[in] item Добавляемый элемент.
  /// @param[in] fr Позиция вставки нового элемента.
  /// @return Возвращает указатель на добавленный элемент
  T* addInSort(T* item, const FindResult& fr);

  /// @brief Функция добавляет копию элемента в отсортированный список
  ///        (флаг сортировки не сбрасывается)
  T* addCopyInSort(const T& item, const FindResult& fr);

  /// @brief Функция назначает список list текущему контейнеру.
  ///
  /// При назначении нового списка - исходный  список  очищается.  Так же пере-
  /// назначаются все  основные  характеристики,  такие  как  capacity,  count,
  /// compare, container, sortState
  void assign(const CustomListType& list);

  /// @brief Функция для расширенной сортировки.
  ///
  /// В функции реализован алгоритм быстрой сортировки.
  /// Функция сортировки может быть вызвана для пустого списка  или для списка
  /// с одним элементом, при этом выставляется соответствующий флаг сортировки.
  /// Это сделано для того чтобы функция addInSort() работала корректно.
  /// @param[in] compare Стратегия  сортировки  (см. класс CompareItem).
  ///            Параметр стратегии сортировки специально сделан не константной
  ///            ссылкой,  это  позволяет  менять  состояние  элемента  compare
  ///            в процессе сортировки.
  /// @param[in] sortMode  Определяет направление  сортировки - по возрастанию
  ///            или убыванию.
  /// @param[in] extParams  Используется  для задания  расширенных  параметров
  ///            сортировки
  template<typename CompareU>
  void sort(CompareU& compare, SortMode sortMode = SortMode::Up,
            const SortExtParams& extParams = SortExtParams());

  /// @brief Функция сортировки, используется стратегия сортировки по умолчанию.
  ///
  void sort(SortMode sortMode = SortMode::Up,
            const SortExtParams& extParams = SortExtParams());

  /// @brief Функция для расширенной сортировки.
  ///
  /// К имени функции добавлен индекс "2", это сделано для того чтобы избежать
  /// конфликта имен в компиляторе от Borland.
  /// Объект стратегии сортировки CompareU создается внутри функции
  template<typename CompareU>
  void sort2(SortMode sortMode = SortMode::Up,
             const SortExtParams& extParams = SortExtParams());

  /// @brief Выполняет обмен данных между списками
  void swap(SelfListType&);

  /*
    Функции временно заблокированы, необходимо тестирование
  */
  //void shift(ShiftMode shiftMode, int shift = 1);
  //void shiftLeft (int shift = 1);  // сдвинуть список влево
  //void shiftRight(int shift = 1);  // сдвинуть список вправо

private:
  typedef typename CustomListType::DataType DataType;
  DataType* d_func() {return CustomListType::d_func();}

  /// Инициализирует основные параметры контейнера, используется в конструкторах
  void init(Container container);

  template<typename CompareU>
  static void qsort(T** sortList, int L, int R, CompareU& compare,
                    SortMode sortMode);
  void grow();

  void setCount(int val) {d_func()->count = val;}
  void setSortState(SortState val) {d_func()->sortState = val;}
  void setAllocator(const Allocator& val) {d_func()->allocator = val;}
  void setContainer(Container val) {d_func()->container = val;}
};

//------------------------ Implementation CustomList -------------------------

#define DECL_IMPL_CUSTLIST_CONSTR \
  template<typename T, typename Compare, typename Allocator> \
  CustomList<T, Compare, Allocator>

#define DECL_IMPL_CUSTLIST(TYPE) \
  template<typename T, typename Compare, typename Allocator> \
  TYPE CustomList<T, Compare, Allocator>

#define DECL_IMPL_CUSTLIST_INTERN_TYPE(TYPE) \
  template<typename T, typename Compare, typename Allocator> \
  typename CustomList<T, Compare, Allocator>::TYPE CustomList<T, Compare, Allocator>

#define DECL_IMPL_CUSTLIST_SUBTMPL1(TYPE, SUBT1) \
  template<typename T, typename Compare, typename Allocator> \
    template<typename SUBT1> \
  TYPE CustomList<T, Compare, Allocator>

#define DECL_IMPL_CUSTLIST_SUBTMPL2(TYPE, SUBT1, SUBT2) \
  template<typename T, typename Compare, typename Allocator> \
    template<typename SUBT1, typename SUBT2> \
  TYPE CustomList<T, Compare, Allocator>

DECL_IMPL_CUSTLIST(int)::indexOf(const T* item) const
{
  int index;
  indexOf2(item, index);
  return index;
}

DECL_IMPL_CUSTLIST(bool)::indexOf2(const T* item, int& index) const
{
  T** it = listBegin();
  T** end = listEnd();
  while (it != end)
  {
    if (*it == item)
    {
      index = int(it - listBegin());
      return true;
    }
    ++it;
  }
  index = -1;
  return false;
}

DECL_IMPL_CUSTLIST_SUBTMPL1(FindResult, U)::find(const U* item,
                                                 const FindExtParams& extParams) const
{
  return find<U, Compare>(item, compare(), extParams);
}

DECL_IMPL_CUSTLIST_SUBTMPL1(T*, U)::findItem(const U* item,
                                             const FindExtParams& extParams) const
{
  return findItem<U, Compare>(item, compare(), extParams);
}

DECL_IMPL_CUSTLIST_SUBTMPL1(FindResult, U)::findRef(const U& item,
                                                    const FindExtParams& extParams) const
{
  return findRef<U, Compare>(item, compare(), extParams);
}

DECL_IMPL_CUSTLIST_SUBTMPL2(FindResult, U, CompareU)::find(const U* item,
                                                           const CompareU& compare,
                                                           const FindExtParams& extParams) const
{
  auto l = [item, &compare](const T* item2) -> int
  {
    return compare(item, item2);
  };
  return findL(l, extParams);
}


DECL_IMPL_CUSTLIST_SUBTMPL2(T*, U, CompareU)::findItem(const U* item,
                                                       const CompareU& compare,
                                                       const FindExtParams& extParams) const
{
  FindResult fr = find<U, CompareU>(item, compare, extParams);
  return fr.success() ? d->list[fr.index()] : nullptr;
}

DECL_IMPL_CUSTLIST_SUBTMPL2(FindResult, U, CompareU)::findRef(const U& item,
                                                              const CompareU& compare,
                                                              const FindExtParams& extParams) const
{
  return find<U, CompareU>(&item, compare, extParams);
}

DECL_IMPL_CUSTLIST_SUBTMPL1(FindResult, CompareL)::findL(const CompareL& compare,
                                                         const FindExtParams& extParams) const
{
  CHECK_INTERNAL_DATA_PTR(d)

  try
  {
    BruteForce bruteForce = extParams.bruteForce;
    int startFindIndex = extParams.startFindIndex;

    if (d->count == 0)
      return FindResult(false, bruteForce, 0);

    if (startFindIndex >= d->count)
      return FindResult(false, BruteForce::Yes, d->count);

    //--- Поиск перебором ---
    if (bruteForce == BruteForce::Yes
        || d->sortState == SortState::Unknown
        || d->sortState == SortState::CustomUp
        || d->sortState == SortState::CustomDown)
        /* || (FCount < 4) Нельзя использовать данное условие для  отсортирован-
              ного списка, т.к. если результат поиска окажется  отрицательным -
              значение в поле FindResult::index() будет непригодно для использо-
              ванияв функции addInSort().  В этом случае при вызове  addInSort()
              возникнет ситуация, когда в результате использования некорректного
              значения FindResult::index()  список  окажется  неотсортированным,
              но флаг состояния сортировки (sortState) при этом сброшен не будет
        */
    {
      T** it = listBegin() + startFindIndex;
      T** end = listEnd();
      // При сравнении it и end не использовать оператор !=,  т.к. потенциально
      // может возникнуть ситуация,  когда it изначально  окажется  больше  end,
      // т.е. если startFindIndex окажется  больше чем число элементов в списке,
      // то условие it != end никогда не наступит
      while (it < /*не использовать оператор != */ end)
      {
        if (compare(*it) == 0)
          return FindResult(true, BruteForce::Yes, int(it - listBegin()));
        ++it;
      }
      return FindResult(false, BruteForce::Yes, d->count);
    }

    //--- Поиск по отсортированному списку ---
    int low = (startFindIndex);
    int high = d->count;
    int mid, result;
    switch (d->sortState)
    {
      case SortState::Up:
        while (true)
        {
          mid = (low + high) >> 1;
          result = compare(d->list[mid]);
          if (result < 0)
            high = mid;
          else if (result > 0)
            //low = mid; Это присвоение приводит к зацикливанию когда начинает
            //           выполняться условие (low + high) / 2 == mid
            low = (low == mid) ? mid + 1 : mid;
          else
            return FindResult(true, BruteForce::No, mid); //совпадение

          //if ((high - low) <= 1) При такой проверке не находятся граничные значения
          if ((high - low) < 1)
            return FindResult(false, BruteForce::No, (result > 0) ? mid + 1 : mid);
        }
      case SortState::Down:
        while (true)
        {
          mid = (low + high) >> 1;
          result = compare(d->list[mid]);
          if (result > 0)
            high = mid;
          else if (result < 0)
            //low = mid; Это присвоение приводит к зацикливанию когда начинает
            //           выполняться условие (low + high) / 2 == mid
            low = (low == mid) ? mid + 1 : mid;
          else
            return FindResult(true, BruteForce::No, mid); //совпадение

          //if ((high - low) <= 1) При такой проверке не находятся граничные значения
          if ((high - low) < 1)
            return FindResult(false, BruteForce::No, (result < 0) ? mid + 1 : mid);
        }
      default:
        // При поиске по отсортированному списку параметр d->sortState может
        // быть либо UpSorted, либо DownSorted. При всех прочих значениях
        // этого параметра поиск должен выполняться перебором
        throw LIST_EXCEPT("Unacceptable value of sortState parameter");
    }
  }
  catch (BreakCompare &)
  {}
  return FindResult(false, BruteForce::Yes, d->count);
}

DECL_IMPL_CUSTLIST_SUBTMPL1(T*, CompareL)::findItemL(const CompareL& compare,
                                                     const FindExtParams& extParams) const
{
  FindResult fr = findL(compare, extParams);
  return fr.success() ? d->list[fr.index()] : nullptr;
}

DECL_IMPL_CUSTLIST_INTERN_TYPE(RangeType)::range(int index1, int index2) const
{
  CHECK_INTERNAL_DATA_PTR(d)

  RangeType r;
  if (d->count == 0)
  {
    r._begin = listEnd();
    r._end   = listEnd();
    return r;
  }

  CHECK_BORDERS(index1)
  CHECK_BORDERS(index2)
  CHECK_NOTLESS(index2, index1)

  r._begin = listBegin() + index1;
  r._end   = listBegin() + index2 + 1;
  return r;
}

DECL_IMPL_CUSTLIST_INTERN_TYPE(RangeType)::range(const FindResultRange& frr) const
{
  if (frr.first.failed() || frr.last.failed())
  {
    RangeType r;
    r._begin = listEnd();
    r._end   = listEnd();
    return r;
  }
  return range(frr.first.index(), frr.last.index());
}

#undef DECL_IMPL_CUSTLIST_CONSTR
#undef DECL_IMPL_CUSTLIST
#undef DECL_IMPL_CUSTLIST_INTERN_TYPE
#undef DECL_IMPL_CUSTLIST_SUBTMPL1
#undef DECL_IMPL_CUSTLIST_SUBTMPL2

//------------------------- Implementation List ------------------------------

#define DECL_IMPL_LIST_CONSTR \
  template<typename T, typename Compare, typename Allocator> \
  List<T, Compare, Allocator>

#define DECL_IMPL_LIST_DESTR  DECL_IMPL_LIST_CONSTR

#define DECL_IMPL_LIST(TYPE) \
  template<typename T, typename Compare, typename Allocator> \
  TYPE List<T, Compare, Allocator>

#define DECL_IMPL_LIST_SUBTMPL1(TYPE, SUBT1) \
  template<typename T, typename Compare, typename Allocator> \
    template<typename SUBT1> \
  TYPE List<T, Compare, Allocator>


DECL_IMPL_LIST_CONSTR::List(Container container)
{
  init(container);
}

DECL_IMPL_LIST_CONSTR::List(const Allocator& allocator, Container container)
{
  init(container);
  setAllocator(allocator);
}

DECL_IMPL_LIST_DESTR::~List()
{
  if (DataType* d = CustomListType::d)
  {
    clear();
    delete [] d->list;
    delete d;
  }
}

DECL_IMPL_LIST(void)::init(Container container)
{
  CustomListType::d = new DataType();
  CustomListType::d->list = nullptr;
  CustomListType::d->count = 0;
  CustomListType::d->capacity = 0;
  CustomListType::d->sortState = SortState::Unknown;
  CustomListType::d->container = container;
}

DECL_IMPL_LIST_CONSTR::List(CustomListType&& list)
{
  CustomListType::d = list.d;
  list.d = nullptr;
}

DECL_IMPL_LIST_CONSTR::List(SelfListType&& list)
{
  CustomListType::d = list.d;
  list.d = nullptr;
}

DECL_IMPL_LIST(T*)::add()
{
  DataType* d = d_func();
  if (d->container == Container::No)
    throw LIST_EXCEPT(ERR_NOCREATE_OBJ);

  // Функция create() должна вызываться без параметров,
  // см. комментарии в типовом аллокаторе
  return add(d->allocator.create(/*0*/));
}

DECL_IMPL_LIST(T*)::add(T* item)
{
  DataType* d = d_func();
  setSortState(SortState::Unknown);
  if (d->count == d->capacity)
    grow();

  d->list[d->count] = item;
  ++d->count;
  return item;
}

DECL_IMPL_LIST(T*)::addCopy(const T& item)
{
  DataType* d = d_func();
  if (d->container == Container::No)
    throw LIST_EXCEPT(ERR_NOCREATE_OBJ);

  return add(d->allocator.create(&item));
}

DECL_IMPL_LIST(void)::clear()
{
  DataType* d = d_func();
  if (d->container == Container::Yes)
  {
    T** it = this->listBegin();
    T** end = this->listEnd();
    while (it != end)
    {
      if (*it)
        d->allocator.destroy(*it);
      ++it;
    }
  }
  d->count = 0;
  d->sortState = SortState::Unknown;
}

DECL_IMPL_LIST(void)::remove(int index, CompressList compressList)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  T *item = release(index, compressList);
  if ((d->container == Container::Yes) && item)
    d->allocator.destroy(item);
}

DECL_IMPL_LIST(int)::removeItem(T* item, CompressList compressList)
{
  int index = -1;
  if (CustomListType::indexOf2(item, index))
    remove(index, compressList);

  return index;
}

DECL_IMPL_LIST(void)::removeLast()
{
  DataType* d = d_func();
  T* item = releaseLast();
  if ((d->container == Container::Yes) && item)
    d->allocator.destroy(item);
}

DECL_IMPL_LIST_SUBTMPL1(void, Condition)::removeCond(const Condition& condition,
                                                     CompressList compressList)
{
  DataType* d = d_func();
  T** it = this->listBegin();
  T** end = this->listEnd();
  while (it != end)
  {
    if (*it && condition(*it))
    {
      if (d->container == Container::Yes)
        d->allocator.destroy(*it);
      *it = nullptr;
    }
    ++it;
  }
  if (compressList == CompressList::Yes)
    this->compressList();
}

DECL_IMPL_LIST(void)::removes(int index, int count, CompressList compressList)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  count = index + count;
  if (count > d->count)
    count = d->count;

  for (int i = index; i < count; ++i)
  {
    T *item = release(i, CompressList::No);
    if ((d->container == Container::Yes) && item)
      d->allocator.destroy(item);
  }
  if (compressList == CompressList::Yes)
      this->compressList();
}

DECL_IMPL_LIST(void)::replace(int index, T* item, bool keepSortState)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  T** it = this->listBegin() + index;
  T* itemOld = *it;
  *it = item;

  if (!keepSortState)
    setSortState(SortState::Unknown);

  if ((d->container == Container::Yes) && itemOld)
    d->allocator.destroy(itemOld);
}

DECL_IMPL_LIST(T*)::release(int index, CompressList compressList)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  // Не используем ф-цию memmove,  так как в некоторых случаях это приводит
  // к утечкам памяти. Выявить закономерности при которых происходят утечки
  // не удалось. Утечки обнаружены с помощью утилиты CodeGuard

  T** it = this->listBegin() + index;
  T* res = *it;
  if (compressList == CompressList::Yes)
  {
    // При сжатии списка сдвигаем все элементы оставшиеся справа от точки
    // удаления элемента на одну позицию влево
    T** end = this->listEnd() - 1;
    while (it != end)
    {
      *it = *(it + 1); ++it;
    }
    --d->count;
  }
  else
    *it = nullptr;

  return res;
}

DECL_IMPL_LIST(int)::releaseItem(T* item, CompressList compressList)
{
  int index = -1;
  if (this->indexOf2(item, index))
    release(index, compressList);

  return index;
}

DECL_IMPL_LIST(T*)::releaseLast()
{
  DataType* d = d_func();
  if (d->count == 0)
    return nullptr;

  T* item = *(this->listEnd() - 1);
  --d->count;
  return item;
}

DECL_IMPL_LIST(void)::exchange(int index1, int index2)
{
  DataType* d = d_func();
  CHECK_BORDERS(index1)
  CHECK_BORDERS(index2)

  setSortState(SortState::Unknown);
  T* item = d->list[index1];
  d->list[index1] = d->list[index2];
  d->list[index2] = item;
}

DECL_IMPL_LIST(T*)::addInSort(T* item, const FindResult& fr)
{
  SortState sortState = this->sortState();
  if ((sortState != SortState::Up) && (sortState != SortState::Down))
    return add(item);

  item = insert(item, fr.index());
  if (!fr.bruteForce())
    setSortState(sortState);

  return item;
}

DECL_IMPL_LIST(T*)::addCopyInSort(const T& item, const FindResult& fr)
{
    DataType* d = d_func();
    if (d->container == Container::No)
      throw LIST_EXCEPT(ERR_NOCREATE_OBJ);

    return addInSort(d->allocator.create(&item), fr);
}

DECL_IMPL_LIST(T*)::insert(int index)
{
  DataType* d = d_func();
  if (d->container == Container::No)
    throw LIST_EXCEPT(ERR_NOCREATE_OBJ);

  // Функция create() должна вызываться без параметров,
  // см. комментарии в типовом аллокаторе
  return insert(d->allocator.create(/*nullptr*/), index);
}

DECL_IMPL_LIST(T*)::insert(T* item, int index)
{
  DataType* d = d_func();
  setSortState(SortState::Unknown);
  if ((index < 0) || (index >= d->count))
    return add(item);

  if (d->count == d->capacity)
    grow();

  // Не используем функцию memmove() (cм. комментарии в теле проц. release())
  T** it = this->listBegin() + index;
  T** end = this->listEnd();
  while (it != end)
  {
    *end = *(end - 1); --end;
  }
  *it = item;
  ++d->count;
  return item;
}

DECL_IMPL_LIST(T*)::insertCopy(const T& item, int index)
{
  DataType* d = d_func();
  if (d->container == Container::No)
    throw LIST_EXCEPT(ERR_NOCREATE_OBJ);

  return insert(d->allocator.create(&item), index);
}

DECL_IMPL_LIST(void)::move(int curIndex, int newIndex)
{
  if (curIndex != newIndex)
  {
    setSortState(SortState::Unknown);
    T* item = release(curIndex);
    insert(item, newIndex);
  }
}

DECL_IMPL_LIST(void)::grow()
{
  DataType* d = d_func();
  int delta = (d->capacity > 64) ? (d->capacity / 2) : 16;
  setCapacity(d->capacity + delta);
}

DECL_IMPL_LIST(void)::setCapacity(int newCapacity)
{
  DataType* d = d_func();
  if (newCapacity > d->capacity)
  {
    T** newList = new T* [newCapacity];
    T** newIt = newList;
    T** it = this->listBegin();
    T** end = this->listEnd();
    while (it != end)
      *newIt++ = *it++;

    delete [] d->list;
    d->list = newList;
    d->capacity = newCapacity;
  }
}

DECL_IMPL_LIST(void)::compressList()
{
  DataType* d = d_func();

  // Удаление пустых элементов без перераспределения памяти
  // См: http://forum.pascal.net.ru/index.php?showtopic=26642

  T** it = this->listBegin();
  T** end = this->listEnd();
  while (true)
  {
    if (it == end)
      return;

    if (*it == nullptr)
    {
      --d->count;
      break;
    }
    ++it;
  }

  T** it2 = it + 1;
  while (it2 != end)
  {
    if (*it2 == nullptr)
    {
      ++it2;
      --d->count;
      continue;
    }
    *it++ = *it2++;
  }
}

DECL_IMPL_LIST(void)::assign(const CustomListType& list)
{
  if (this == &list)
      return;

  // Отладить
  break_point

  clear();
  DataType* d = d_func();
  //
  // Нельзя выполнять присвоение через разыменование d, в этом случае емкость
  // списка будет установлена неверно
  // *d = *(list.d);
  //
  setCapacity(list.capacity());
  setCompare(list.compare());
  setAllocator(list.allocator());
  setContainer(list.container());
  setCount(list.count());
  setSortState(SortState::Unknown);

  T** srcIt  = list.listBegin();
  T** srcEnd = list.listEnd();
  T** dstIt  = this->listBegin();
  int count  = 0;
  while (srcIt != srcEnd)
  {
    try
    {
      *dstIt = (d->container == Container::Yes)
               ? d->allocator.create(*srcIt) : *srcIt;
      ++dstIt, ++srcIt, ++count;
    }
    catch (...)
    {
      d->count = count;
      throw;
    }
  }
  setSortState(list.sortState());
}

DECL_IMPL_LIST_SUBTMPL1(void, CompareU)::qsort(T** sortList,
                                               int L, int R,
                                               CompareU& compare,
                                               SortMode sortMode)
{
  int i = L;
  int j = R;
  T *tmp, *x = sortList[(L + R) >> 1];
  do
  {
    switch (sortMode)
    {
      case SortMode::Up:
        while ((compare(sortList[i], x) < 0) && (i < R)) ++i;
        while ((compare(sortList[j], x) > 0) && (j > L)) --j;
        break;
      default: // SortMode::Down
        while ((compare(sortList[i], x) > 0) && (i < R)) ++i;
        while ((compare(sortList[j], x) < 0) && (j > L)) --j;
    }
    if (i <= j)
    {
      tmp = sortList[i];
      sortList[i] = sortList[j];
      sortList[j] = tmp;
      ++i; if (j > 0) --j;
    }
  } while (i <= j);

  if (L < j) qsort(sortList, L, j, compare, sortMode);
  if (i < R) qsort(sortList, i, R, compare, sortMode);
}

DECL_IMPL_LIST_SUBTMPL1(void, CompareU)::sort(CompareU& compare,
                                              SortMode sortMode,
                                              const SortExtParams& extParams)
{
  // Выставляем флаг сортировки даже если в списке один элемент
  // или нет элементов вовсе. Это необходимо для корректной работы
  // функции addInSort()
  setSortState((sortMode == SortMode::Up) ? SortState::Up : SortState::Down);

  DataType* d = d_func();
  if (d->list && (d->count > 1))
  {
    try
    {
      int loSortBorder = extParams.loSortBorder;
      int hiSortBorder = extParams.hiSortBorder;

      if (!inRange(loSortBorder, 0, d->count))
        loSortBorder = 0;

      if (!inRange(hiSortBorder, loSortBorder + 1, d->count + 1))
        hiSortBorder = d->count;

      if ((loSortBorder != 0) || (hiSortBorder != d->count))
        setSortState((sortMode == SortMode::Up) ? SortState::CustomUp
                                                : SortState::CustomDown);

      qsort(d->list, loSortBorder, hiSortBorder - 1, compare, sortMode);
    }
    catch (BreakCompare&)
    {
      setSortState(SortState::Unknown);
    }
    catch (...)
    {
      setSortState(SortState::Unknown);
    }
  }
}

DECL_IMPL_LIST_SUBTMPL1(void, CompareU)::sort2(SortMode sortMode,
                                               const SortExtParams& extParams)
{
  CompareU compare;
  sort<CompareU>(compare, sortMode, extParams);
}

DECL_IMPL_LIST(void)::sort(SortMode sortMode,
                           const SortExtParams& extParams)
{
  sort<Compare>(CustomListType::compare(), sortMode, extParams);
}

DECL_IMPL_LIST(void)::swap(SelfListType& list)
{
  DataType* p = list.d;
  list.d = CustomListType::d;
  CustomListType::d = p;
}

/*
  Функции временно заблокированы, необходимо тестирование
*/
//template<typename T, typename Compare, typename Allocator>
//void List<T, Compare, Allocator>::shift(ShiftMode shiftMode, int shift)
//{
//#ifndef NDEBUG
//  CHECK_NOTLESS_(shift, 1)
//#endif
//
//  if (shiftMode == lst::ShiftLeft)
//    shiftLeft(shift);
//  else
//    shiftRight(shift);
//}

//template<typename T, typename Compare, typename Allocator>
//void List<T, Compare, Allocator>::shiftLeft(int shift)
//{
//  #ifndef NDEBUG
//  CHECK_NOTLESS_(shift, 1)
//  #endif
//
//  set_SortState(NoSorted);
//  T **List_ = getList();
//  T **ListTmp = new T* [shift];
////  memmove((void*)&ListTmp[0], (void*)&List_[0], Shift * sizeof(T*));
////  memmove((void*)&List_[0], (void*)&List_[Shift], (get_Count() - Shift) * sizeof(T*));
////  memmove((void*)&List_[get_Count() - Shift], (void*)&ListTmp[0], Shift * sizeof(T*));
//  memmove((void*)&ListTmp[0], (void*)&List_[0], shift * sizeof(T*));
//  memmove((void*)&List_[0], (void*)&List_[shift], (getCount() - shift) * sizeof(T*));
//  memmove((void*)&List_[getCount() - shift], (void*)&ListTmp[0], shift * sizeof(T*));
//  delete [] ListTmp;
//}

//template<typename T, typename Compare, typename Allocator>
//void List<T, Compare, Allocator>::shiftRight(int shift)
//{
//  #ifndef NDEBUG
//  CHECK_NOTLESS_(shift, 1)
//  #endif
//
//  set_SortState(NoSorted);
//  T **List_ = getList();
//  T **ListTmp = new T* [shift];
////  memmove((void*)&ListTmp[0], (void*)&List_[get_Count() - Shift], Shift * sizeof(T*));
////  memmove((void*)&List_[Shift], (void*)&List_[0], (get_Count() - Shift) * sizeof(T*));
////  memmove((void*)&List_[0], (void*)&ListTmp[0], Shift * sizeof(T*));
//  memmove((void*)&ListTmp[0], (void*)&List_[getCount() - shift], shift * sizeof(T*));
//  memmove((void*)&List_[shift], (void*)&List_[0], (getCount() - shift) * sizeof(T*));
//  memmove((void*)&List_[0], (void*)&ListTmp[0], shift * sizeof(T*));
//  delete [] ListTmp;
//}

//------------------------ Implementation Functions --------------------------

template<typename ListT, typename CompareL>
FindResult find(const ListT& list, const CompareL& compare)
{
  for (decltype(list.size()) i = 0; i < list.size(); ++i)
    if (compare(&list.at(i)) == 0)
      return FindResult(true, BruteForce::Yes, i);

  return FindResult(false, BruteForce::Yes, list.size());
}

template<typename ListT, typename CompareL>
auto findItem(const ListT& list, const CompareL& compare) -> typename ListT::pointer
{
  FindResult fr = find<ListT, CompareL>(list, compare);
  return fr.success() ? const_cast<typename ListT::pointer>(&list.at(fr.index()))
                      : nullptr;
}

//---
template<typename T, typename ListT, typename CompareT>
FindResult find(const T* item, const ListT& list, const CompareT& compare)
{
  auto l = [item, &compare](typename ListT::const_pointer item2) -> int
  {
    return compare(item, item2);
  };
  return find(list, l);
}

template<typename T, typename ListT, typename CompareT>
FindResult findRef(const T& item, const ListT& list, const CompareT& compare)
{
  return find<T, ListT, CompareT>(&item, list, compare);
}

template<typename T, typename ListT, typename CompareT>
T* findItem(const T* item, const ListT& list, const CompareT& compare)
{
  FindResult fr = find<T, ListT, CompareT>(item, list, compare);
  return fr.success() ? const_cast<T*>(&list.at(fr.index())) : nullptr;
}

//---
template<typename ListT, typename CompareL>
FindResult firstFindResultL(const ListT& list, const CompareL& compare,
                            const FindResult& fr)
{
  if (fr.success())
  {
    int i = fr.index();
    for (; i >= 0; --i)
    {
      if (compare(&list.at(i)) != 0)
        break;
    }
    return FindResult(true, BruteForce::Yes, i + 1);
  }
  return fr;
}

template<typename ListT, typename CompareL>
FindResult lastFindResultL(const ListT& list, const CompareL& compare,
                           const FindResult& fr)
{
  if (fr.success())
  {
    int i = fr.index();
    for (; i < list.count(); ++i)
    {
      if (compare(&list.at(i)) != 0)
        break;
    }
    return FindResult(true, BruteForce::Yes, i - 1);
  }
  return fr;
}

template<typename ListT, typename CompareT>
FindResult firstFindResult(const ListT& list, const CompareT& compare,
                           const FindResult& fr)
{
  if (fr.failed())
    return fr;

  const typename ListT::ValueType* item = &list.at(fr.index());
  auto l = [item, &compare](const typename ListT::ValueType* item2) -> int
  {
    return compare(item, item2);
  };
  return firstFindResultL(list, l, fr);
}

template<typename ListT, typename CompareT>
FindResult lastFindResult(const ListT& list, const CompareT& compare,
                          const FindResult& fr)
{
  if (fr.failed())
    return fr;

  const typename ListT::ValueType* item = &list.at(fr.index());
  auto l = [item, &compare](const typename ListT::ValueType* item2) -> int
  {
    return compare(item, item2);
  };
  return lastFindResultL(list, l, fr);
}

//---
template<typename ListT, typename CompareL>
FindResultRange rangeFindResultL(const ListT& list, const CompareL& compare,
                                 const FindResult& fr)
{
  FindResultRange frr;
  frr.first = firstFindResultL(list, compare, fr);
  frr.last  = lastFindResultL (list, compare, fr);
  return frr;
}

template<typename ListT, typename CompareT>
FindResultRange rangeFindResult(const ListT& list, const CompareT& compare,
                                const FindResult& fr)
{
  FindResultRange frr;
  frr.first = firstFindResult(list, compare, fr);
  frr.last  = lastFindResult (list, compare, fr);
  return frr;
}

#undef DECL_IMPL_LIST_CONSTR
#undef DECL_IMPL_LIST_DESTR
#undef DECL_IMPL_LIST
#undef DECL_IMPL_LIST_SUBTMPL1

#undef LIST_EXCEPT

#undef CHECK_BORDERS
#undef CHECK_NOTLESS
#undef CHECK_INTERNAL_DATA_PTR

} // namespace lst
