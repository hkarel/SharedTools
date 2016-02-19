/* clang-format off */
/*****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализован класс-список (List) с доступом к элементам по индексу.

  Реализованы механизмы:
    - быстрой сортировки
    - частичной сортировки
    - быстрого поиска
    - грубого поиска
    - возможность добавлять элементы в список без нарушения порядка сортировки

*****************************************************************************/

#pragma once

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif

#include <exception>
#include <string.h>
#include "break_point.h"

namespace lst {

//#if defined(_MSC_VER)
//#define DECLSPEC_SELECTANY_LST extern "C" __declspec(selectany)
//#else
//#define DECLSPEC_SELECTANY_LST constexpr
//#endif

//DECLSPEC_SELECTANY_LST const bool CONTAINER_CLASS = true;
//DECLSPEC_SELECTANY_LST const bool NO_CONTAINER_CLASS = !CONTAINER_CLASS;
//DECLSPEC_SELECTANY_LST const bool REFERENCE_CLASS = !CONTAINER_CLASS;

//DECLSPEC_SELECTANY_LST const bool COMPRESS_LIST = true;
//DECLSPEC_SELECTANY_LST const bool NO_COMPRESS_LIST = !COMPRESS_LIST;

constexpr bool CONTAINER_CLASS = true;
constexpr bool NO_CONTAINER_CLASS = !CONTAINER_CLASS;
constexpr bool REFERENCE_CLASS = !CONTAINER_CLASS;

constexpr bool COMPRESS_LIST = true;
constexpr bool NO_COMPRESS_LIST = !COMPRESS_LIST;

enum class BruteForce {No = 0, Yes = 1};


/// @brief Флаги направления сортировки.
enum SortMode
{
  SortDown = 0,  /// Сортировать по убыванию
  SortUp = 1     /// Сортировать по возрастанию
};

/// @brief Флаги сдвига списка.
enum ShiftMode
{
  ShiftLeft = 0,  /// Сдвинуть элементы списка влево
  ShiftRight = 1  /// Сдвинуть элементы списка вправо
};

/// @brief Флаги состояний сортировки.
enum SortState
{
  NoSorted = 0,         /// Список находится в не отсортированном состоянии
  UpSorted = 1,         /// Список отсортирован по возрастанию
  DownSorted = 2,       /// Список отсортирован по убыванию
  CustomUpSorted = 3,   /// Список частично отсортирован по возрастанию
  CustomDownSorted = 4  /// Список частично отсортирован по убыванию
};


class FindResult;

template<typename ListT, typename CompareT>
FindResult find(const ListT&, const CompareT&);

template<typename ListT, typename CompareL>
FindResult firstFindResultL(const ListT&, const CompareL&, const FindResult&);

template<typename ListT, typename CompareL>
FindResult lastFindResultL(const ListT&, const CompareL&, const FindResult&);

template <typename, typename, typename> class CustomList;
template <typename, typename, typename> class List;


/// @brief Функция проверяет попадает ли значение index в диапазон minVal-maxVal,
/// значение minVal включается в диапазон проверки, а maxVal - нет.
template<typename T>
inline bool inRange(T index, T minVal, T maxVal)
{
  return ((index >= minVal) && (index < maxVal));
}

template<typename ListType>
inline bool checkBorders(ListType &list, int index)
{
  return ((index >= 0) && (index < list.size()));
}


/**
  @brief Класс ListExcept.
  Используется для обработки исключений.
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
  virtual const char* what() const NOEXCEPT {return _msg;}
};

/*DECLSPEC_SELECTANY_LST */
constexpr const char* ERR_NOCREATEOBJ =
  "Impossible create object of the class. (NO_CONTAINER_CLASS)";

#if defined(_MSC_VER)
#define LIST_EXCEPT(MSG) ListExcept(MSG, __FUNCTION__)
#else
#define LIST_EXCEPT(MSG) ListExcept(MSG, __func__)
#endif

#ifndef NDEBUG
#  define CHECK_BORDERS(index) \
     if ((index < 0) || (index >= d->count)) throw LIST_EXCEPT("Index out of bounds");

#  define CHECK_NOTLESS(index, less) \
     if (index < less) throw LIST_EXCEPT("Index must not be less than "#less);

#  define CHECK_INTERNAL_DATA_PTR(D_) \
     if (D_ == 0) throw LIST_EXCEPT("Internal data-pointer is null");
#else
#  define CHECK_BORDERS(index)
#  define CHECK_NOTLESS(index, less_)
#  define CHECK_INTERNAL_DATA_PTR(D_)
#endif // NDEBUG


/**
  @brief Класс BreakCompare.
  Используется в функциях/стратегиях сравнения элементов
  для прерырания процессов сортировки или поиска.
*/
struct BreakCompare {};


/**
  @brief Класс FindResult - результат функций поиска.
  Для отсортированного списка:
    если элемент найден, то success() == true, а index() возвращает позицию
    элемента в списке;
    если элемент не найден, то success() == false, а index() возврвщает позицию
    в которую можно вставить ненайденный элемент и список при этом останется
    отсортированным. Для добавления элемента в отсортированный список используется
    функция List::addInSort()
  Для неотсортированного списка:
    если элемент найден, то success() == true, а index() возвращает позицию
    элемент в списке;
    если элемент не найден, то success() == false, а значение index() возвращает
    количество элементов в списке (count).
*/
class FindResult
{
public:
  FindResult() : _index(-1), _success(false), _bruteForce(false) {}
  bool success()    const NOEXCEPT {return  _success;}
  bool failed()     const NOEXCEPT {return !_success;}
  int  index()      const NOEXCEPT {return _index;}
  bool bruteForce() const NOEXCEPT {return _bruteForce;}

  explicit operator bool() const NOEXCEPT {return _success;}

private:
  int      _index;
  unsigned _success    : 1;
  unsigned _bruteForce : 1;
  unsigned _reserved   : 30;

  FindResult(bool success, BruteForce bruteForce, int index)
    : _index(index), _success(success), _bruteForce(unsigned(bruteForce))
  {}
  template <typename, typename, typename> friend class CustomList;

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
  @brief Сервисная структура, используется для агрегации расширенных
         параметров поиска.
*/
struct FindExtParams
{
  /// Указатель на дополнительные параметры передаваемые в функцию поиска.
  /// Если в качестве стратегии поиска используется не функтор, а обычная функ-
  /// ция, то extParam - это единственный способ передать в эту функцию внешние
  /// данные.
  void *extParam = {0};

  /// Определяет поиск простым перебором (метод грубой силы).
  /// Если bruteForce = Yes поиск будет происходить простым перебором, даже
  /// для отсортированного списка.
  BruteForce bruteForce = {BruteForce::No};

  /// Индекс с которого начинается поиск.
  /// Если startFindIndex >= list::count(), то функция поиска вернет результат
  /// со статусом FindResult::failed() = TRUE. При этом если элемент с данным
  /// статусом будет добавлен в отсортированный список через функцию addInSort(),
  /// то флаг сортировки будет сброшен (см. пояснения к функции list::addInSort()).
  int startFindIndex = {0};

  FindExtParams() = default;
  explicit FindExtParams(BruteForce bruteForce)
    : extParam(0), bruteForce(bruteForce), startFindIndex(0)
  {}
};


/**
  @brief Сервисная структура, используется для агрегации расширенных
         параметров сортировки.
*/
struct SortExtParams
{
  /// Используется для передачи в функцию сортировки дополнительных параметров.
  /// Применяется, как правило, если в качестве стратегии сортировки используется
  /// не функтор, а обычная функция.
  void *extParam = {0};

  /// loSortBorder, hiSortBorder Границы сортировки, позволяют производить
  /// сортировку по указанному диапазону. При назначении диапазона соблюдаются
  /// следующие требования:
  /// 1) 0 <= loSortBorder < count(), в противном случае LoSortBorder
  ///    выставляется в 0.
  /// 2) loSortBorder < hiSortBorder <= count(), в противном случае
  ///    hiSortBorder выставляется в count (количество элементов в списке).
  /// Значения по умолчанию равные -1 предполагают сортировку по всему диапазону.
  int loSortBorder = {0};
  int hiSortBorder = {-1};

  SortExtParams() = default;
  SortExtParams(void *extParam, int loSortBorder = 0, int hiSortBorder = -1)
    : extParam(extParam), loSortBorder(loSortBorder), hiSortBorder(hiSortBorder)
  {}
};

/**
  @brief Функции выполняют поиск перебором (грубый поиск).

  Первые две функции выполнены для использования с lambda-функциями с сигнатурой:
  int [](const ListT::ValueType* item) где item элемент списка;
  следующие три функции в качестве compare-элемента используют функцию или функтор
  со следующей сигнатурой:
  int function(const T* item1, const ListT::ValueType* item2, void* extParam).
  Дополнительные пояснения по параметру extParam можно посмотреть в описании
  к параметру FindExtParams::extParam.
  Примечание: в качестве контейнера ListT можно использовать std::vector.
*/
template<typename ListT, typename CompareL>
FindResult find(const ListT& list, const CompareL& compare);

template<typename ListT, typename CompareL>
auto findItem(const ListT& list, const CompareL& compare) -> typename ListT::pointer;

//---
template<typename T, typename ListT, typename CompareT>
FindResult find(const T* item, const ListT& list, const CompareT& compare,
                void* extParam = 0);

template<typename T, typename ListT, typename CompareT>
FindResult findRef(const T& item, const ListT& list, const CompareT& compare,
                   void* extParam = 0);

template<typename T, typename ListT, typename CompareT>
T* findItem(const T* item, const ListT& list, const CompareT& compare,
            void* extParam = 0);


/**
  @brief Группа функций выполняет поиск первого или последнего элемента в после-
         довательности одинаковых значений.

  Если список содержит не уникальные значения, то при сортировке данного списка
  одинаковые значения будут идти друг за другом. При использовании функций быст-
  рого поиска в таком списке - найденное значение скорее всего будет не первым и
  не последним в последовательности одинаковых значений. Тем не менее наиболее
  часто нужно именно первое или последнее значение в последовательности.
  Данные функции как раз выполняют поиск первого/последнего элемента в последо-
  вательности одинаковых значений.
  В качестве результата возвращается индекс первого/последнего элемента в после-
  довательности.
*/

/// Примечание: в качестве compare-элемента используется lambda-функция с сигна-
/// турой: int [](const ListT::ValueType* item) где item элемент списка.
template<typename ListT, typename CompareL>
FindResult firstFindResultL(const ListT& list, const CompareL& compare,
                            const FindResult& fr);

template<typename ListT, typename CompareL>
FindResult lastFindResultL(const ListT& list, const CompareL& compare,
                           const FindResult& fr);

/// Примечание: в качестве compare-элемента используется функция или функтор
/// с сигнатурой:
/// int function(const ListT::ValueType* item1, const ListT::ValueType* item2,
///              void* extParam).
template<typename ListT, typename CompareT>
FindResult firstFindResult(const ListT& list, const CompareT& compare,
                           const FindResult& fr, void* extParam = 0);

template<typename ListT, typename CompareT>
FindResult lastFindResult(const ListT& list, const CompareT& compare,
                          const FindResult& fr, void* extParam = 0);

/**
  @brief Группа функций выполняет поиск первого и последнего элемента в после-
         довательности одинаковых значений.
*/
template<typename ListT, typename CompareL>
FindResultRange rangeFindResultL(const ListT& list, const CompareL& compare,
                                 const FindResult& fr);

template<typename ListT, typename CompareT>
FindResultRange rangeFindResult(const ListT& list, const CompareT& compare,
                                const FindResult& fr, void* extParam = 0);


/**
  @brief Макрос используется в классе-стратегии сортировки и поиска.
*/
#define LIST_COMPARE_ITEM(ITEM1, ITEM2) ((ITEM1 > ITEM2) ? 1 : ((ITEM1 < ITEM2) ? -1 : 0))

/**
  @brief Макрос используется в классе-стратегии сортировки и поиска в тех случаях,
         когда сортировка или поиск выполняются по нескольким полям.
         Ниже приведен пример использования. Здесь сортировка выполняется по трем
         полям с убывающим приоритетом сравнения от field1 к field3.
         struct Compare
         {
           int operator() (const Type* item1, const Type* item2, void*) const
           {
             LIST_COMPARE_MULTI_ITEM( item1->field1, item2->field1)
             LIST_COMPARE_MULTI_ITEM( item1->field2, item2->field2)
             return LIST_COMPARE_ITEM(item1->field3, item2->field3);
           }
         };
*/
#define LIST_COMPARE_MULTI_ITEM(ITEM1, ITEM2) \
   {if (ITEM1 > ITEM2) return  1; else if (ITEM1 < ITEM2) return -1;}


/**
  @brief Класс-стратегия используется для сортировки и поиска.
*/
template<typename T> struct CompareItem
{
  /// @param[in] item1 Первый сравниваемый элемент.
  /// @param[in] item2 Второй сравниваемый элемент.
  /// @param[in] extParam Параметр требуется для совместимости сигнатуры
  ///     вызова operator () в функциях CustomList::find() и List::sort().
  ///     Как правило этот параметр используется когда в качестве стратегии
  ///     сравнения применяется не функтор, а обычная функция.
  ///     См. так же описание к функциям CustomList::find(), List::sort().
  /// @return Результат сравнения.
  /// Примечание: Оператор не должен быть виртуальным. Если оператор сделать
  /// виртуальным, то для каждого инстанциируемого класса придется определять
  /// операторы "<", ">", "==", что сводит "на нет" идею класса-стратегии
  /// применительно к сортировке.
  int operator() (const T* item1, const T* item2, void* /*extParam*/) const
  {
    return LIST_COMPARE_ITEM(*item1, *item2);
  }
};

/**
  @brief Фиктивный класс-стратегия используется в тех случаях, когда нужно
  явно указать, что никакие стратегии сортировки использовать не планируется.
*/
struct CompareItemDummy {};


/**
  @brief Распределитель памяти для элементов списка
*/
template<typename T> struct AllocatorItem
{
  /// @brief Функция создания объектов.
  ///
  /// Примечания:
  /// 1. Должно быть две функции create(). Если использовать только одну
  ///    функцию вида T* create(const T* x = 0) {return (x) ? new T(*x) : new T();}
  ///    то компилятор будет требовать обязательное наличие конструктора копирования
  ///    у инстанциируемого класса.
  /// 2. Функции create()/destroy() должны быть неконстантными, так как их вызов
  ///    в конкретных реализациях может приводить к изменению состояния экземпляра
  ///    распределителя памяти.
  T* create() {return new T();}
  T* create(const T* x) {return (x) ? new T(*x) : new T();}

  /// @brief Функция разрушения элементов.
  void destroy(T* x) {
    static_assert(sizeof(T) > 0, "Can't delete pointer to incomplete type");
    delete x;
  }
};


/**
  @brief Класс CustomList

  Этот класс не является потокобезопасным, следует с осторожностью допускать
  одновременный вызов методов данного класса из разных потоков.
*/
template <
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
  /// @return Индекс искомого элемента, в случае неудачи возвращает -1.
  int indexOf(const T* item) const;

  /// @brief Функция поиска по адресу элемента.
  ///
  /// @param[in]  item  Искомый элемент.
  /// @param[out] index В случае удачи возвращает индекс искомого элемента,
  /// @return В случае удачного поиска возвращает TRUE.
  bool indexOf2(const T* item, int& index) const;

  /// @brief Функция проверяет границы списка.
  ///
  /// Если index удовлетворяет условию 0 <= index < count(), то функция
  /// возвращает TRUE, в противном случае функция возвращает FALSE.
  bool checkBorders(int index) const {return inRange(index, 0, d->count);}

  /// @brief Функции поиска.
  ///
  /// @param[in] item Искомый элемент.
  /// @param[in] extParams Используется для задания расширенных параметров
  ///            поиска.
  /// @return Структура с результатом поиска.
  template<typename U>
  FindResult find(const U* item,
                  const FindExtParams& extParams = FindExtParams()) const;

  /// @brief Перегруженные функции поиска, определена для удобства использования.
  ///
  /// @return Возвращает указатель на искомый элемент, если элемент
  /// не найден - возвращает 0.
  template<typename U>
  T* findItem(const U* item,
              const FindExtParams& extParams = FindExtParams()) const;

  /// @brief Перегруженные функции поиска, определена для удобства использования.
  ///
  /// @param[in] item Искомый элемент передаваемый по ссылке.
  template<typename U>
  FindResult findRef(const U& item,
                     const FindExtParams& extParams = FindExtParams()) const;

  /// @brief Перегруженные функции поиска, определены для удобства использовния.
  ///
  /// Позволяет выполнять поиск со стратегией поиска отличной от той,
  /// что была определена в классе-контейнере.
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
  /// int [](const List::ValueType* item)
  template<typename CompareL>
  FindResult findL(const CompareL& compare,
                   const FindExtParams& extParams = FindExtParams()) const;

  template<typename CompareL>
  T* findItemL(const CompareL& compare,
               const FindExtParams& extParams = FindExtParams()) const;

  /// @brief Доступ к элементу списка по индексу.
  ///
  /// @return Возвращает указатель на элемент.
  T* item(int index) const {CHECK_BORDERS(index); return d_func()->list[index];}

  /// @brief Доступ к элементу списка по индексу.
  ///
  /// @return Возвращает ссылку на элемент.
  T&       itemRef(int index) {return *item(index);}
  const T& itemRef(int index) const {return *item(index);}

  /// @brief Доступ к элементу списка по индексу.
  ///
  /// @return Возвращает ссылку на элемент, аналогичен функции itemRef().
  T&       operator[] (int index) {return *item(index);}
  const T& operator[] (int index) const {return *item(index);}

  /// @brief Доступ к элементу списка по индексу.
  ///
  /// @return Возвращает ссылку на элемент, введена для совместимости с STL.
  const T& at(int index) const {return *item(index);}

  /// @brief Возвращает указатель на адрес первого элемента в линейном массиве
  /// указателей на элементы.
  T** listBegin() const {return d_func()->list;}

  /// @brief Возвращает указатель на адрес идущий за последним элементом
  /// в линейном массиве указателей на элементы.
  T** listEnd() const {return (d_func()->list + d_func()->count);}

  /// @brief Возвращает указатель на адрес первого элемента в линейном массиве
  /// указателей на элементы.
  /// Используется в конструкции вида: for (T* t : List)
  T** begin() const {return listBegin();}

  /// @brief Возвращает указатель на адрес идущий за последним элементом
  /// в линейном массиве указателей на элементы.
  /// Используется в конструкции вида: for (T* t : List)
  T** end() const {return listEnd();}

  /// @brief Возвращает количество элементов в списке.
  int count() const {return d_func()->count;}

  /// @brief Возвращает зарезервированную длинну массива list()
  /// выделенную для работы списка.
  int capacity() const {return d_func()->capacity;}

  /// @brief Признак того, что список является контейнером
  ///
  /// Если container() == true, то при разрушении или очистке списка
  /// все элементы списка будут автоматически разрушены,
  /// если container() == false - элементы списка разрушены не будут.
  bool container() const {return d_func()->container;}

  /// @brief Определяет состояние сортировки.
  SortState sortState() const {return d_func()->sortState;}

  /// @brief Возвращает ссылку на стратегию сортировки.
  Compare& compare() {return d_func()->compare;}

  /// @brief Возвращает константную ссылку на стратегию сортировки.
  const Compare& compare() const {return d_func()->compare;}

  /// @brief Возвращает ссылку на распределитель памяти.
  Allocator& allocator() {return d_func()->allocator;}

  /// @brief Возвращает константную ссылку на распределитель памяти.
  const Allocator& allocator() const {return d_func()->allocator;}

  /// @brief Возвращает количество элементов в списке,
  /// введено для совместимости с STL.
  int size() const {return count();}

  /// @brief Возвращает true если список пуст.
  bool empty() const {return (count() == 0);}

  /// @brief Возвращает первый элемент в списке.
  /// Если список пустой, то возвращает нуль.
  T* first() const {return (d_func()->count) ? d->list[0] : 0;}

  /// @brief Возвращает последний элемент в списке.
  /// Если список пустой, то возвращает нуль.
  T* last() const {return (d_func()->count) ? d->list[d->count - 1] : 0;}

  template<typename IteratorT> class Range
  {
  public:
    IteratorT** begin() const {return _begin;}
    IteratorT** end()   const {return _end;}
  private:
    Range() {}
    IteratorT** _begin;
    IteratorT** _end;
    template <typename, typename, typename> friend class CustomList;
  };
  typedef Range<T> RangeType;

  /// @brief Сервисные функции, возвращают структуру Range содержащую пару
  ///        итераторов указывающих на первый элемент и на элемент идущий
  ///        за последним для заданного диапазона значений.
  ///
  /// Основное назначение этих функций - это использование в конструкции
  /// вида: for (T* t : range).
  /// @param[in] index1 Индекс первого элемента в диапазоне.
  /// @param[in] index2 Индекс последнего элемента в диапазоне.
  /// @return Структура Range определяющая диапазон.
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
    DataT**   list      = {0};
    int       count     = {0};
    int       capacity  = {0};
    SortState sortState = {NoSorted};
    bool      container = {CONTAINER_CLASS};
    Compare   compare;
    Allocator allocator;
  };
  typedef Data<T> DataType;
  DataType* d;

  DataType* d_func() const {CHECK_INTERNAL_DATA_PTR(d); return d;}
  template <typename, typename, typename> friend class List;
};


/**
  @brief Класс List
  Этот класс не является потокобезопасным, поэтому не следует допускать
  одновременный вызов методов данного класса из разных потоков.
*/
template <
  typename T,
  typename Compare = CompareItem<T>,
  typename Allocator = AllocatorItem<T>
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
  explicit List(bool container = CONTAINER_CLASS);
  explicit List(const Allocator&, bool container = CONTAINER_CLASS);

  List(CustomListType&&);
  List(SelfListType&&);

  ~List();

  List(const SelfListType&) = delete;
  SelfListType& operator= (SelfListType&&) = delete;
  SelfListType& operator= (const SelfListType&) = delete;

  /// @brief Добавляет новый элемент T в конец списка.
  ///
  /// Класс элемента должен иметь конструктор по умолчанию.
  /// @return Возвращает указатель на добавленны элемент.
  T* add();

  /// @brief Добавляет уже созданный элемент T в конец списка.
  ///
  /// @param[in] item Добавляемый элемент.
  /// @return Возвращает указатель на добавленный элемент.
  T* add(T* item);
  //T* add(const T& item, bool /*copy*/);

  /// @brief Добавляет копию элемента T в конец списка.
  /// @return Возвращает указатель на добавленный элемент.
  T* addCopy(const T& item);

  /// @brief Добавляет новый элемент T в указанную позицию в списке.
  ///
  /// Если index больше количества элементов в списке (count()) - элемент будет
  /// добавлен в конец списка. Класс элемента T должен иметь конструктор по умолчанию.
  /// @param[in] index Позиция вставки нового элемента.
  /// @return Возвращает указатель на добавленный элемент.
  T* insert(int index = 0);

  /// @brief Добавляет существующий элемент T в указанную позицию в списке.
  ///
  /// Если index больше количества элементов в списке (count()) - элемент будет
  /// добавлен в конец списка. Класс элемента T должен иметь конструктор по умолчанию.
  /// @param[in] item Добавляемый элемент.
  /// @param[in] index Позиция вставки нового элемента.
  /// @return Возвращает указатель на добавленный элемент.
  T* insert(T* item, int index = 0);
  //T* insert(T* item, bool /*copy*/, int index = 0);

  /// @brief Добавляет копию элемента T в указанную позицию в списке.
  T* insertCopy(const T& item, int index = 0);

  /// @brief Удаляет элемент из списка.
  ///
  /// При удалении элемента из списка происходит его разрушение.
  /// @param[in] index Индекс удаляемого элемента.
  /// @param[in] compressList Признак сжатия списка. Если compressList = false,
  ///     то ячейкам в массиве списка присваивается 0. Таким образом получается
  ///     разряженный список содержащий нуль-значения.
  void remove(int index, bool compressList = true);

  /// @brief Удаляет элемент из списка.
  ///
  /// При удалении элемента из списка происходит его разрушение.
  /// @param[in] item Элемент удаляемый из списка.
  /// @param[in] compressList Признак сжатия списка.
  /// @return Индекс удаленного элемента, если элемент не присутствовал
  /// в списке возвращает int(-1).
  int removeItem(T* item, bool compressList = true);

  /// @brief Удаляет последний элемент из списка.
  ///
  /// При удалении элемента из списка происходит его разрушение.
  void removeLast();

  /// @brief Удаляет элементы из списка согласно условию condition.
  ///
  /// При удалении элементов из списка происходит их разрушение.
  /// @param[in] cond Функор или функция с сигнатурой bool condition(T*).
  /// @param[in] compressList Признак сжатия списка.
  template<typename Condition>
  void removeCond(const Condition& condition, bool compressList = true);

  /// @brief Удаляет элементы из списка.
  ///
  /// При удалении элементов из списка происходит их разрушение.
  /// @param[in] index Индекс с которого начнется удаление элементов.
  /// @param[in] count Количество удаляемых элементов.
  /// @param[in] compressList Признак сжатия списка.
  void removes(int index, int count, bool compressList = true);

  /// @brief Заменяет элемент в списке.
  ///
  /// @param[in] index Индекс в котором будет произведена замена.
  /// @param[in] item Новый элемент в списке.
  /// @param[in] keepSortState Признак сохранения флага сортировки.
  void replace(int index, T* item, bool keepSortState = false);

  /// @brief Удаляет элемент из списка, при этом разрушения элемента не происходит.
  ///
  /// @param[in] index Индекс удаляемого элемента
  /// @param[in] compressList Признак сжатия списка (см. описание в функции remove()).
  /// @return Возвращает указатель на удаленный из списка элемент.
  T* release(int index, bool compressList = true);

  /// @brief Удаляет элемент из списка, при этом разрушения элемента не происходит.
  ///
  /// @param[in] item Удаляемый элемент.
  /// @param[in] compressList Признак сжатия списка (см. описание в функции remove()).
  /// @return В случае успешного удаления возвращает индекс удаленного элемента.
  int releaseItem(T* item, bool compressList = true);

  /// @brief Удаляет последний элемент из списка при этом разрушения элемента
  /// не происходит. Если в списке нет элементов будет возвращен 0.
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
  T*   releaseLast();

  /// @brief Меняет местами элементы в списке.
  ///
  /// @param[in] index1 Индекс первого элемента.
  /// @param[in] index2 Индекс второго элемента.
  void exchange(int index1, int index2);

  /// @brief Перемещает элемент из позиции curIndex в позицию newIndex.
  void move(int curIndex, int newIndex);

  /// @brief Очищает список элементов.
  //void clear() {CustomListType::clear();}
  void clear(); // {CustomListType::internalClear();}


  /// @brief Устанавливает новый размер массива указателей на элементы списка.
  ///
  /// Указатель на массив указателей элементов списка можно получить через
  /// функцию listBegin().
  /// @param[in] newCapacity Новая длинна массива указателей.
  void setCapacity(int newCapacity);

  /// @brief Изменяет глобальную стратегию сортировки и поиска.
  void setCompare(const Compare& val) {d_func()->compare = val;}

  /// @brief Функция сжатия списка.
  ///
  /// Удаляет нулевые ячейки, которые могли образоваться в массиве указателей
  /// на элементы при вызове методов remove() или release() с параметром
  /// compressList = false.
  void compressList();

  /// @brief Функция добавляет элемент в отсортированный список
  ///        (флаг сортировки не сбрасывается).
  ///
  /// Предполагается, что добавление нового элемента не нарушит порядок сортиров-
  /// ки элементов в отсортированном списке. Для того чтобы порядок сортировки не
  /// нарушался индекс вставки должен быть корректным.
  /// Корректный индекс можно определить при помощи функции find() и структуры
  /// FindResult.
  /// Если элемент не найден в списке, то в структуре FindResult будет возвращен
  /// индекс для корректного добавления элемента в список.
  /// Если флаг сортировки не определен (список не отсортирован), то элемент будет
  /// добавлен в конец списка. При частичной сортировке - флаг состояния сортиров-
  /// ки будет сброшен, а элемент будет добавлен в конец списка.
  /// ВНИМАНИЕ: Если поиск производится с флагом bruteForce = TRUE, то при добав-
  /// лении нового элемента через функцию addInSort() флаг состояния сортировки
  /// будет сброшен.
  /// @param[in] item Добавляемый элемент.
  /// @param[in] fr Позиция вставки нового элемента.
  /// @return Возвращает указатель на добавленный элемент.
  T* addInSort(T* item, const FindResult& fr);
  //T* addInSort(T* item, const FindResult& fr, bool /*copy*/);

  /// @brief Функция добавляет копию элемента в отсортированный список
  ///        (флаг сортировки не сбрасывается).
  T* addCopyInSort(const T& item, const FindResult& fr);

  /// @brief Функция назначает список list текущему контейнеру.
  ///
  /// При назначении нового списка - исходный список очищается. Так же пере-
  /// назначаются все основные характеристики, такие как capacity, count,
  /// compare, container, sortState.
  void assign(const CustomListType& list);

  /// @brief Функция для расширенной сортировки.
  ///
  /// В функции реализован алгоритм быстрой сортировки.
  /// Функция сортировки может быть вызвана для пустого списка или для списка
  /// с одним элементом, при этом выставляется соответствующий флаг сортировки.
  /// Это сделано для того чтобы функция addInSort() могла отрабатывать корректно.
  /// @param[in] compare Стратегия сортировки (в качестве примера см. класс CompareItem).
  ///            Параметр стратегии сортировки специально сделан не константной
  ///            ссылкой, это дает возможность менять состояние элемента compare
  ///            в процессе сортировки.
  /// @param[in] sortMode Определяет направление сортировки - по возрастанию
  ///            или убыванию.
  /// @param[in] extParams Используется для задания расширенных параметров
  ///            сортировки.
  template<typename CompareU>
  void sort(CompareU& compare, SortMode sortMode = SortUp,
            const SortExtParams& extParams = SortExtParams());

  /// @brief Функция сортировки, используется стратегия сортировки по умолчанию.
  ///
  void sort(SortMode sortMode = SortUp,
            const SortExtParams& extParams = SortExtParams());

  /// @brief Функция для расширенной сортировки.
  ///
  /// К имени функции добавлен индекс "2", это сделано для того чтобы
  /// избежать конфликта имен в компиляторе от Borland.
  /// Объект стратегии сортировки CompareU создается внутри функции.
  template<typename CompareU>
  void sort2(SortMode sortMode = SortUp,
             const SortExtParams& extParams = SortExtParams());

  /// @brief Выполняет обмен данных между списками.
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

  // Инициализирует основные параметры контейнера, используется в конструкторах.
  void init(bool container /*, const Allocator &*/);

  //template<typename CompareU>
  //void QuickSort(T** sortList, int L, int R,
  //               CompareU& compare, SortMode sortMode, void *extParam);
  template<typename CompareU>
  void QSort(T** sortList, int L, int R,
             CompareU& compare, SortMode sortMode, void *extParam);

  void grow();

  void setCount(int val) {d_func()->count = val;}
  void setSortState(SortState val) {d_func()->sortState = val;}
  void setAllocator(const Allocator& val) {d_func()->allocator = val;}
  void setContainer(bool val) {d_func()->container = val;}

};


//------------------------ Implementation CustomList ------------------------

#define DECL_IMPL_CUSTLIST_CONSTR \
  template<typename T, typename Compare, typename Allocator> \
  CustomList<T, Compare, Allocator>

#define DECL_IMPL_CUSTLIST_DESTR  DECL_IMPL_CUSTLIST_CONSTR

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
  auto l = [item, &compare, &extParams](const T* item2) -> int
  {
    return compare(item, item2, extParams.extParam);
  };
  return findL(l, extParams);
}


DECL_IMPL_CUSTLIST_SUBTMPL2(T*, U, CompareU)::findItem(const U* item,
                                                       const CompareU& compare,
                                                       const FindExtParams& extParams) const
{
  FindResult fr = find<U, CompareU>(item, compare, extParams);
  return fr.success() ? d->list[fr.index()] : 0;
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

    // Функция поиска перебором
    auto bruteFind = [this, &compare, startFindIndex] () -> FindResult
    {
      T** it = listBegin() + startFindIndex;
      T** end = listEnd();
      // При сравнении it и end не использовать оператор !=, т.к. потенциально
      // может возникнуть ситуация, когда it изначально окажется больше end,
      // т.е. если startFindIndex окажется  больше чем число элементов в списке,
      // то условие it != end никогда не наступит.
      while (it < /*не использовать оператор != */ end)
      {
        if (compare(*it) == 0)
          return FindResult(true, BruteForce::Yes, int(it - listBegin()));
        ++it;
      }
      return FindResult(false, BruteForce::Yes, d->count);
    };

    if (/*(FCount < 4)*/ // Нельзя использовать данное условие для отсортированного
                         // списка, т.к. результат поиска (FindResult::index()) будет
                         // содержать неверное значение. Использование данного значения
                         // в функции addInSort() для отсортированного списка приведет
                         // к тому, что список окажется неотсортированным, но флаг
                         // состояния сортировки (sortState) при этом сброшен не будет.
      bruteForce == BruteForce::Yes
      || (d->sortState == NoSorted)
      || (d->sortState == CustomUpSorted)
      || (d->sortState == CustomDownSorted))
    {
      return bruteFind();
    }

    int low = (startFindIndex);
    int high = d->count;
    int mid;
    int result;
    SortState sort_state = (bruteForce == BruteForce::Yes) ? NoSorted : d->sortState;
    switch (sort_state)
    {
      case UpSorted:
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
      case DownSorted:
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
        return bruteFind();
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
  return fr.success() ? d->list[fr.index()] : 0;
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
#undef DECL_IMPL_CUSTLIST_DESTR
#undef DECL_IMPL_CUSTLIST
#undef DECL_IMPL_CUSTLIST_INTERN_TYPE
#undef DECL_IMPL_CUSTLIST_SUBTMPL1
#undef DECL_IMPL_CUSTLIST_SUBTMPL2



//------------------------- Implementation List -----------------------------

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

#define DECL_LIST_SELFTYPE List<T, Compare, Allocator>


DECL_IMPL_LIST_CONSTR::List(bool container)
{
  init(container);
}

DECL_IMPL_LIST_CONSTR::List(const Allocator& allocator, bool container)
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

DECL_IMPL_LIST(void)::init(bool container/*, const Allocator& allocator*/)
{
  CustomListType::d = new DataType();
  CustomListType::d->list = 0;
  CustomListType::d->count = 0;
  CustomListType::d->capacity = 0;
  CustomListType::d->sortState = NoSorted;
  CustomListType::d->container = container;
  //CustomListType::d->allocator = allocator;
}

DECL_IMPL_LIST_CONSTR::List(CustomListType&& list)
{
  CustomListType::d = list.d;
  list.d = 0;
}

DECL_IMPL_LIST_CONSTR::List(SelfListType&& list)
{
  CustomListType::d = list.d;
  list.d = 0;
}

//DECL_IMPL_LIST(DECL_LIST_SELFTYPE &)::operator= (const CustomListType& list)
//{
//  assign(list);
//  return *this;
//}

//DECL_IMPL_LIST(DECL_LIST_SELFTYPE &)::operator= (const SelfListType& list)
//{
//  assign(list);
//  return *this;
//}

DECL_IMPL_LIST(T*)::add()
{
  DataType* d = d_func();
  if (!d->container)
    throw LIST_EXCEPT(ERR_NOCREATEOBJ);

  // Функция create() должна вызываться без параметров,
  // см. комментарии в типовом аллокаторе.
  return add(d->allocator.create(/*0*/));
}

DECL_IMPL_LIST(T*)::add(T* item)
{
  DataType* d = d_func();
  setSortState(NoSorted);
  if (d->count == d->capacity)
    grow();

  d->list[d->count] = item;
  ++d->count;
  return item;
}

DECL_IMPL_LIST(T*)::addCopy(const T& item)
{
  DataType* d = d_func();
  if (!d->container)
    throw LIST_EXCEPT(ERR_NOCREATEOBJ);

  return add(d->allocator.create(&item));
}

DECL_IMPL_LIST(void)::clear()
{
  DataType* d = d_func();
  if (d->container)
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
  d->sortState = NoSorted;
}

DECL_IMPL_LIST(void)::remove(int index, bool compressList)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  T *item = release(index, compressList);
  if (d->container && item)
    d->allocator.destroy(item);
}

DECL_IMPL_LIST(int)::removeItem(T* item, bool compressList)
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
  if (d->container && item)
    d->allocator.destroy(item);
}

DECL_IMPL_LIST_SUBTMPL1(void, Condition)::removeCond(const Condition& condition,
                                                     bool compressList)
{
  DataType* d = d_func();
  T** it = this->listBegin();
  T** end = this->listEnd();
  while (it != end)
  {
    if (*it && condition(*it))
    {
      if (d->container)
        d->allocator.destroy(*it);
      *it = 0;
    }
    ++it;
  }
  if (compressList)
    this->compressList();
}

DECL_IMPL_LIST(void)::removes(int index, int count, bool compressList)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  count = index + count;
  if (count > d->count)
    count = d->count;

  for (int i = index; i < count; ++i)
  {
    T *item = release(i, NO_COMPRESS_LIST);
    if (d->container && item)
      d->allocator.destroy(item);
  }
  if (compressList)
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
    setSortState(NoSorted);

  if (d->container && itemOld)
    d->allocator.destroy(itemOld);
}

DECL_IMPL_LIST(T*)::release(int index, bool compressList)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  // Не используем ф-цию memmove, т.к. в некоторых случаях это приводит к утечкам
  // памяти. Выявить закономерности при которых происходят утечки не удалось.
  // Утечки обнаружены с помощью утилиты CodeGuard.

  T** it = this->listBegin() + index;
  T* res = *it;
  if (compressList)
  {
    // При сжатии списка сдвигаем все элементы оставшиеся справа
    // от точки удаления элемента на одну позицию влево
    T** end = this->listEnd() - 1;
    while (it != end)
    {
      *it = *(it + 1); ++it;
    }
    --d->count;
  }
  else
    *it = 0;

  return res;
}

DECL_IMPL_LIST(int)::releaseItem(T* item, bool compressList)
{
  int index = -1;
  if (this->indexOf2(item, index))
    release(index, compressList);

  return index;
}

DECL_IMPL_LIST(T*)::releaseLast()
{
  DataType* d = d_func();
  if (d->count == 0) return 0;
  T* item = *(this->listEnd() - 1);
  --d->count;
  return item;
}

DECL_IMPL_LIST(void)::exchange(int index1, int index2)
{
  DataType* d = d_func();
  CHECK_BORDERS(index1)
  CHECK_BORDERS(index2)

  setSortState(NoSorted);
  T* item = d->list[index1];
  d->list[index1] = d->list[index2];
  d->list[index2] = item;
}

DECL_IMPL_LIST(T*)::addInSort(T* item, const FindResult& fr)
{
  SortState sortState = this->sortState();
  //if ((sort_state == NoSorted)
  //     || (sort_state == CustomUpSorted)
  //     || (sort_state == CustomDownSorted))
  if ((sortState != UpSorted) && (sortState != DownSorted))
    return add(item);

  item = insert(item, fr.index());
  if (!fr.bruteForce())
    setSortState(sortState);

  return item;
}

DECL_IMPL_LIST(T*)::addCopyInSort(const T& item, const FindResult& fr)
{
    DataType* d = d_func();
    if (!d->container)
      throw LIST_EXCEPT(ERR_NOCREATEOBJ);

    return addInSort(d->allocator.create(&item), fr);
}

DECL_IMPL_LIST(T*)::insert(int index)
{
  DataType* d = d_func();
  if (!d->container)
    throw LIST_EXCEPT(ERR_NOCREATEOBJ);

  // Функция create() должна вызываться без параметров,
  // см. комментарии в типовом аллокаторе.
  return insert(d->allocator.create(/*0*/), index);
}

DECL_IMPL_LIST(T*)::insert(T* item, int index)
{
  DataType* d = d_func();
  setSortState(NoSorted);
  if ((index < 0) || (index >= d->count))
    return add(item);

  if (d->count == d->capacity)
    grow();

  // Не используем функцию memmove() (cм. комментарии в теле проц. release()).
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
  if (!d->container)
    throw LIST_EXCEPT(ERR_NOCREATEOBJ);

  return insert(d->allocator.create(&item), index);
}

DECL_IMPL_LIST(void)::move(int curIndex, int newIndex)
{
  if (curIndex != newIndex)
  {
    setSortState(NoSorted);
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

  // TODO: Рассмотреть задачу с удалением пустых элементов используя только
  // один вектор. См: http://forum.pascal.net.ru/index.php?showtopic=26642

  T** temp_list = new T* [d->capacity];
  T** temp_item = temp_list;
  T** item_ = CustomListType::listBegin();
  T** end_  = CustomListType::listEnd();
  int count_ = d->count;
  while (item_ != end_)
  {
    if (*item_ != 0)
    {
      *temp_item++ = *item_++;
    }
    else
    {
      ++item_, --count_;
    }
  }
  delete [] d->list;
  d->list = temp_list;
  d->count = count_;
}

DECL_IMPL_LIST(void)::assign(const CustomListType& list)
{
  if (this == &list)
      return;

  // Отладить
  break_point

  clear();

  // !!! Так делать нельзя, поэтому выполняем присвоение параметров через функции
  //*d = *(list.d);
  // !!!
  setCapacity(list.capacity());
  setCompare(list.compare());
  setAllocator(list.allocator());
  setContainer(list.container());
  setCount(list.count());
  setSortState(NoSorted);

  T** srcIt = list.listBegin();
  T** srcEnd = list.listEnd();
  T** dstIt = this->listBegin();
  int count = 0;
  DataType* d = d_func();
  while (srcIt != srcEnd)
  {
    try
    {
      *dstIt = (d->container) ? d->allocator.create(*srcIt) : *srcIt;
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

// --- Код оставлен в качестве примера ---
//DECL_IMPL_LIST_SUBTMPL1(void, CompareU)::QuickSort(T** sortList,
//                                                   int L, int R,
//                                                   CompareU& compare,
//                                                   SortMode sortMode,
//                                                   void* extParam)
//{
//  register int I, J;
//  T *P1, *T1;
//  do
//  {
//    I = L; J = R;
//    P1 = sortList[(L + R) >> 1];
//    do
//    {
//    	switch (sortMode)
//    	{
//  	  	case SortUp:
//          while (compare(sortList[I], P1, extParam) < 0) ++I;
//          while (compare(sortList[J], P1, extParam) > 0) --J;
//          break;
//        default:
//          while (compare(sortList[I], P1, extParam) > 0) ++I;
//          while (compare(sortList[J], P1, extParam) < 0) --J;
//      }
//      if (I <= J)
//      {
//        T1 = sortList[I];
//        sortList[I++] = sortList[J];
//        sortList[J--] = T1;
//      }
//    } while (I < J);
//    if (L < J) QuickSort(sortList, L, J, compare, sortMode, extParam);
//    L = I;
//  } while (I <= R);
//}

DECL_IMPL_LIST_SUBTMPL1(void, CompareU)::QSort(T** sortList,
                                               int L, int R,
                                               CompareU& compare,
                                               SortMode sortMode,
                                               void* extParam)
{
  int i = L;
  int j = R;
  T *tmp, *x = sortList[(L + R) >> 1];
  do
  {
    switch (sortMode)
    {
      case SortUp:
        while ((compare(sortList[i], x, extParam) < 0) && (i < R)) ++i;
        while ((compare(sortList[j], x, extParam) > 0) && (j > L)) --j;
        break;
      default: //SortDown
        while ((compare(sortList[i], x, extParam) > 0) && (i < R)) ++i;
        while ((compare(sortList[j], x, extParam) < 0) && (j > L)) --j;
    }
    if (i <= j)
    {
      tmp = sortList[i];
      sortList[i] = sortList[j];
      sortList[j] = tmp;
      ++i; if (j > 0) --j;
    }
  } while (i <= j);

  if (L < j) QSort(sortList, L, j, compare, sortMode, extParam);
  if (i < R) QSort(sortList, i, R, compare, sortMode, extParam);
}

DECL_IMPL_LIST_SUBTMPL1(void, CompareU)::sort(CompareU& compare,
                                              SortMode sortMode,
                                              const SortExtParams& extParams)
{
  // Выставляем флаг сортировки даже если в списке один элемент
  // или нет элементов вовсе. Это необходимо для корректной работы
  // функции addInSort().
  setSortState((sortMode == SortUp) ? UpSorted : DownSorted);

  DataType* d = d_func();
  if (d->list && (d->count > 1))
  {
    try
    {
      int loSortBorder = extParams.loSortBorder;
      int hiSortBorder = extParams.hiSortBorder;

      //setSortState((sortMode == SortUp) ? UpSorted : DownSorted);
      if (!inRange(loSortBorder, 0, d->count))
        loSortBorder = 0;

      if (!inRange(hiSortBorder, loSortBorder + 1, d->count + 1))
        hiSortBorder = d->count;

      if ((loSortBorder != 0) || (hiSortBorder != d->count))
        setSortState((sortMode == SortUp) ? CustomUpSorted : CustomDownSorted);

      QSort<CompareU>(d->list, loSortBorder, hiSortBorder - 1,
                      compare, sortMode, extParams.extParam);
    }
    catch (BreakCompare &)
    {
      setSortState(NoSorted);
    }
    catch (...)
    {
      setSortState(NoSorted);
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


//------------------------ Implementation Functions -------------------------

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
  return fr.success() ? const_cast<typename ListT::pointer>(&list.at(fr.index())) : 0;
}

//---
template<typename T, typename ListT, typename CompareT>
FindResult find(const T* item, const ListT& list, const CompareT& compare,
                void* extParam)
{
  auto l = [item, &compare, extParam](typename ListT::const_pointer item2) -> int
  {
    return compare(item, item2, extParam);
  };
  return find(list, l);
}

template<typename T, typename ListT, typename CompareT>
FindResult findRef(const T& item, const ListT& list, const CompareT& compare,
                   void* extParam)
{
  return find<T, ListT, CompareT>(&item, list, compare, extParam);
}

template<typename T, typename ListT, typename CompareT>
T* findItem(const T* item, const ListT& list, const CompareT& compare,
            void* extParam)
{
  FindResult fr = find<T, ListT, CompareT>(item, list, compare, extParam);
  return fr.success() ? const_cast<T*>(&list.at(fr.index())) : 0;
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
                           const FindResult& fr, void* extParam)
{
  if (fr.failed())
    return fr;

  const typename ListT::ValueType* item = &list.at(fr.index());
  auto l = [item, &compare, extParam](const typename ListT::ValueType* item2) -> int
  {
    return compare(item, item2, extParam);
  };
  return firstFindResultL(list, l, fr);
}

template<typename ListT, typename CompareT>
FindResult lastFindResult(const ListT& list, const CompareT& compare,
                          const FindResult& fr, void* extParam)
{
  if (fr.failed())
    return fr;

  const typename ListT::ValueType* item = &list.at(fr.index());
  auto l = [item, &compare, extParam](const typename ListT::ValueType* item2) -> int
  {
    return compare(item, item2, extParam);
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
                                const FindResult& fr, void* extParam)
{
  FindResultRange frr;
  frr.first = firstFindResult(list, compare, fr, extParam);
  frr.last  = lastFindResult (list, compare, fr, extParam);
  return frr;
}

#undef DECL_IMPL_LIST_CONSTR
#undef DECL_IMPL_LIST_DESTR
#undef DECL_IMPL_LIST
#undef DECL_IMPL_LIST_SUBTMPL1
#undef DECL_LIST_SELFTYPE

#undef LIST_EXCEPT

#undef CHECK_BORDERS
#undef CHECK_NOTLESS
#undef CHECK_INTERNAL_DATA_PTR

#undef DECLSPEC_SELECTANY_LST

} // namespace lst
