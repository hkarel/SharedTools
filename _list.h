/****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализован класс-список (List) с доступом к элементам
  по индексу. Имеется возможность доступа к элементам через примитивный
  итератор.

  Реализованы механизмы:
    - быстрой сортировки
    - частичной сортировки
    - быстрого поиска
    - грубого поиска
    - возможность добавлять элементы в список без нарушения порядка сортировки

****************************************************************************/

#pragma once

#include <exception>
#include <string.h>
#include "break_point.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wmultichar"

namespace lst
{

#if defined(_MSC_VER)
#define DECLSPEC_SELECTANY_LST extern "C" __declspec(selectany)
#else
#define DECLSPEC_SELECTANY_LST constexpr
#endif

DECLSPEC_SELECTANY_LST const bool CONTAINER_CLASS = true;
DECLSPEC_SELECTANY_LST const bool REFERENCE_CLASS = false;
DECLSPEC_SELECTANY_LST const bool NO_CONTAINER_CLASS = REFERENCE_CLASS;

DECLSPEC_SELECTANY_LST const bool COMPRESS_LIST = true;
DECLSPEC_SELECTANY_LST const bool BRUTE_FORCE = true;


/// @brief Флаги направления сортировки.
enum SortMode
{
  SortDown = 0,
  SortUp = 1
};

/// @brief Флаги сдвига списка.
enum ShiftMode
{
  ShiftLeft = 0,
  ShiftRight = 1
};

/// @brief Флаги состояний сортировки.
enum SortState
{
  NoSorted = 0,
  UpSorted = 1,
  DownSorted = 2,
  CustomUpSorted = 3,
  CustomDownSorted = 4
};


class FindResult;

template<typename T, typename ListT, typename CompareT>
FindResult bruteFind(const T*, const ListT&, const CompareT&, void* = 0);

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
    //memcpy(msg, msg, strlen(msg));
  }
  virtual const char* what() const noexcept {return _msg;}
};

DECLSPEC_SELECTANY_LST const char* ERR_NOCREATEOBJ =
  "Impossible create object of the class. (NO_CONTAINER_CLASS)";

#if defined(__BORLANDC__)
#define  LIST_FUNC_NAME  __FUNC__
#elif defined(_MSC_VER)
#define  LIST_FUNC_NAME  __FUNCTION__
#else
//#define  LIST_FUNC_NAME  __PRETTY_FUNCTION__
#define  LIST_FUNC_NAME  __func__
#endif


#ifdef __BORLANDC__
#  ifdef NO_USE_VCL_EXCEPT
#    define LIST_EXCEPT(MSG) ListExcept(MSG, LIST_FUNC_NAME)
#  else
#    define LIST_EXCEPT(MSG) Exception
#  endif
#else
#  define LIST_EXCEPT(MSG) ListExcept(MSG, LIST_FUNC_NAME)
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
#endif //NDEBUG



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
  bool success()    const {return  _success;}
  bool failed()     const {return !_success;}
  int  index()      const {return _index;}
  bool bruteForce() const {return _bruteForce;}
  operator bool()   const {return _success;}

private:
  int      _index;
  unsigned _success    : 1;
  unsigned _bruteForce : 1;
  unsigned _reserved   : 30;

  FindResult(bool success, bool bruteForce, int index)
    : _index(index), _success(success), _bruteForce(bruteForce)
  {}
  template <typename, typename, typename> friend class CustomList;
  template <typename T, typename ListT, typename CompareT>
  friend FindResult bruteFind(const T*, const ListT&, const CompareT&, void*);
};



/**
  @brief Функции выполняют поиск перебором (грубый поиск).
*/
template<typename T, typename ListT, typename CompareT>
FindResult bruteFind(const T* t, const ListT& list, const CompareT& compare, void* extParam)
{
  for (int i = 0; i < list.size(); ++i)
    if (compare(t, list.at(i), extParam) == 0)
      return FindResult(true, BRUTE_FORCE, i);

  return FindResult(false, BRUTE_FORCE, list.size());
}

template<typename CompareT, typename T, typename ListT>
FindResult bruteFind(const T* t, const ListT& list, void* extParam = 0)
{
  CompareT compare;
  return bruteFind<T, ListT, CompareT>(t, list, compare, extParam);
}

template<typename T, typename ListT, typename CompareT>
FindResult bruteFindRef(const T& t, const ListT& list, const CompareT& compare, void* extParam = 0)
{
  return bruteFind<T, ListT, CompareT>(&t, list, compare, extParam);
}

template<typename CompareT, typename T, typename ListT>
FindResult bruteFindRef(const T& t, const ListT& list, void* extParam = 0)
{
  CompareT compare;
  return bruteFind<T, ListT, CompareT>(&t, list, compare, extParam);
}
//---------------------------------------------------------------------------



/**
  @brief Выполняет поиск первого элемента в последовательности одинаковых значений.

  Если список содержит неуникальные значения, то при сортировке такого списка
  одинаковые значения будут идти друг за другом. При использовании функций быст-
  рого поиска в таком списке найденное значение наиболее часто будет не первым
  в последовательности одинаковых значений. Тем не менее наиболее часто нужно
  именно первое значение в последовательности. Данная функция как раз и выполняет
  поиск первого элемента в последовательности одинаковых значений.
*/
// !!! Для функции нужно unit-тестирование (реализация временно закоментированна) !!!
template<typename ListT, typename CompareT>
int beginingFindResult2(const ListT& list,
                        const CompareT& compare,
                        const FindResult& fr, void* extParam = 0);


/**
  @brief Макрос используется в классе-стратегии сортировки и поиска.
*/
#define LIST_COMPARE_ITEM(ITEM1, ITEM2) ((ITEM1 > ITEM2) ? 1 : ((ITEM1 < ITEM2) ? -1 : 0))


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
  /// Примечание: Если оператор сделать виртуальным, то для каждого
  /// инстанциируемого класса придется определять операторы "<", ">", "==",
  /// что сводит "на нет" идею класса-стратегии применительно к сортировке.
  /*virtual*/ int operator() (const T* item1, const T* item2, void* extParam) const
  {
    //if (*item1> *item2) return  1;
    //else if (*item1 < *item2) return -1;
    //return 0;
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
  /// Примечание: должно быть две функции create(). Если использовать только
  /// одну функцию вида T* create(const T* x = 0){return (x) ? new T(*x) : new T();}
  /// то компилятор будет требовать обязательное наличие конструктора копирования
  /// у инстанциируемого класса.
  /// Примечание: функции create()/destroy() должны быть неконстантными,
  /// так как их вызов в конкретных реализациях может приводить к изменению
  /// состояния экземпляра распределителя памяти.
  T* create() /*const*/ {return new T();}
  T* create(const T* x) /*const*/ {return (x) ? new T(*x) : new T();}

  /// @brief Функция разрушения элементов.
  void destroy(T* x) /*const*/ {delete x;}
};


/**
  @brief Итератор, используется для перебора элементов списка только "вперед".
  Итератор не является потокобезопасным. Основное назначение: более быстрая
  альтернатива конструкции for (int i = 0; i < list.count(); ++i) {}
*/
template<typename ListT> class Iterator
{
public:
  typedef typename ListT::ValueType  ValueType;

public:
  ValueType* first() const {return *_begin;}
  ValueType* last() const {return *(_end - 1);}
  ValueType* operator-> () const {return *_item;}
  bool next() {return (++_item != _end);}

private:
  Iterator();
  // При создании итератора текущий элемент устанавливается на позицию
  // перед первым элементом. Это сделано для того, что бы при первом
  // вызове функции next() позиционироваться на первый элемент списка.
  Iterator(const ListT& list)
  {
    _begin = list.listBegin();
    _end   = list.listEnd();
    _item  = _begin - 1;
  }

private:
  ValueType** _begin;
  ValueType** _end;
  ValueType** _item;

  template <typename, typename, typename> friend class CustomList;
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
  typedef T* PointerType;
  typedef T  ValueType;
  typedef Compare  CompareType;
  typedef Allocator  AllocatorType;
  typedef CustomList<T, Compare, Allocator>  CustomListType;
  typedef Iterator<CustomListType> IteratorType;

  // Для совместимости с STL
  typedef T* pointer;
  typedef T& reference;
  typedef T  value_type;

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

  /// @brief Функция поиска.
  ///
  /// @param[in] item Искомый элемент.
  /// @param[in] bruteForce Определяет поиск простым перебором (метод грубой силы)
  ///     Если bruteForce = TRUE поиск будет происходить простым перебором, даже
  ///     для отсортированного списка.
  /// @param[in] startFindIndex Индекс с которого начинается поиск.
  ///     Если startFindIndex >= count(), то функция поиска вернет FALSE и флаг
  ///     сортировки будет сброшен при добавлении элемента через функцию addInSort().
  /// @param[in] extParam Указатель на дополнительные параметры передаваемые в
  ///     функцию сортировки. Если в качестве стратегии сортировки/поиска исполь-
  ///     зуется не функтор, а обычная функция, то extParam - это единственный
  ///     способ передать в эту функцию внешние данные (см. комментарии к sort()).
  /// @return Структура с результатом поиска.
  template<typename U>
  FindResult find(const U* item, bool bruteForce = false,
                  int startFindIndex = 0, void* extParam = 0) const;

  /// @brief Перегруженная функция, определена для удобства использования.
  ///
  /// @return Возвращает указатель на искомый элемент, если элемент
  /// не найден - возвращает 0.
  template<typename U>
  T* findItem(const U* item, bool bruteForce = false,
              int startFindIndex = 0, void* extParam = 0) const;

  /// @brief Перегруженная функция, определена для удобства использования.
  ///
  /// @param[in] item Искомый элемент передаваемый по ссылке.
  template<typename U>
  FindResult findRef(const U& item, bool bruteForce = false,
                     int startFindIndex = 0, void* extParam = 0) const;

  template<typename U>
  T* findRefItem(const U& item, bool bruteForce = false,
                 int startFindIndex = 0, void* extParam = 0) const;

//  /// @brief Перегруженные функции, определены для удобства использования.
//  ///
//  /// !!! Конфликтует со следующим блоком функций поиска !!!
//  /// Позволяет выполнять поиск со стратегией поиска отличной от той,
//  /// что была определена в классе-контейнере.
//  /// Объект стратегии поиска UCompare создается внутри функции.
//  template<typename UCompare, typename U>
//  FindResult find(const U* item, bool bruteForce = false,
//                  int startFindIndex = 0, void* extParam = 0) const;

//  template<typename UCompare, typename U>
//  T* findItem(const U* item, bool bruteForce = false,
//              int startFindIndex = 0, void* extParam = 0) const;

//  template<typename UCompare, typename U>
//  FindResult findRef(const U& item, bool bruteForce = false,
//                     int startFindIndex = 0, void* extParam = 0) const;

//  template<typename UCompare, typename U>
//  T* findRefItem(const U& item, bool bruteForce = false,
//                 int startFindIndex = 0, void* extParam = 0) const;

  /// @brief Перегруженные функции, определены для удобства использовния.
  ///
  /// Позволяет выполнять поиск со стратегией поиска отличной от той,
  /// что была определена в классе-контейнере.
  template<typename U, typename UCompare>
  FindResult find(const U* item, const UCompare& compare, bool bruteForce = false,
                  int startFindIndex = 0, void* extParam = 0) const;

  template<typename U, typename UCompare>
  T* findItem(const U* item, const UCompare& compare, bool bruteForce = false,
              int startFindIndex = 0, void* extParam = 0) const;

  template<typename U, typename UCompare>
  FindResult findRef(const U& item, const UCompare& compare, bool bruteForce = false,
                     int startFindIndex = 0, void* extParam = 0) const;

  template<typename U, typename UCompare>
  T* findRefItem(const U& item, const UCompare& compare, bool bruteForce = false,
                 int startFindIndex = 0, void* extParam = 0) const;


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

  IteratorType iterator() const {return IteratorType(*this);}

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
    DataT**   list;
    int       count;
    int       capacity;
    Compare   compare;
    Allocator allocator;
    SortState sortState;
    bool      container;
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
  typedef List<T, Compare, Allocator>  SelfListType;
  typedef CustomList<T, Compare, Allocator>  CustomListType;

  typedef typename CustomListType::PointerType    PointerType;
  typedef typename CustomListType::ValueType      ValueType;
  typedef typename CustomListType::CompareType    CompareType;
  typedef typename CustomListType::AllocatorType  AllocatorType;

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
  /// @param[in] item Элемент удаляемый из списка.
  /// @param[in] compressList Признак сжатия списка.
  /// @return Индекс удаленного элемента, если элемент не присутствовал
  /// в списке возвращает int(-1).
  int removeItem(T* item, bool compressList = true);

  /// @brief Удаляет последний элемент из списка.
  void removeLast();

  /// @brief Удаляет элементы из списка.
  ///
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

  /// @brief Перегруженная функция, (перегруженная функция).
  ///
  /// @param[in] item Удаляемый элемент.
  /// @param[in] compressList Признак сжатия списка (см. описание в функции remove()).
  /// @return В случае успешного удаления возвращает TRUE.
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
  /// нарушался индекс вставки должен быть корректным - это должен контролировать
  /// пользователь.
  /// Корректный индекс можно определить при помощи функции find() и структуры
  /// FindResult.
  /// Если элемент не найден в списке, то в стрктуре FindResult будет возвращен
  /// индекс для корректного добавления элемента в список.
  /// Если флаг сортировки не определен (список не отсортирован), то элемент будет
  /// добавлен в конец списка. При частичной сортировке - флаг состояния сортиров-
  /// ки будет сброшен а элемент будет добавлен в конец списка.
  /// ВНИМАНИЕ: Если поиск производится с флагом bruteForce = TRUE, то при
  /// добавлении нового элемента через функцию addInSort() флаг состояния сорти-
  /// ровки будет сброшен.
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
  /// @param[in] compare Cтратегия сортировки (в качестве примера см. класс CompareItem).
  ///     Пареметр стратегии сортировки специально сделан неконстантной ссылкой,
  ///     это дает возможность менять состояние элемента compare в процессе
  ///     сортировки.
  /// @param[in] sortMode Определяет направление сортировки - по возрастанию
  ///     или убыванию.
  /// @param[in] extParam Используется для передачи в функцию сортировки
  ///     дополнительных параметров. Применяется, как правило, если в качестве
  ///     стратегии сортировки используется не функтор, а обычная функция
  ///     (см. комментраии к find()).
  /// @param[in] loSortBorder, hiSortBorder Границы сортировки, позволяют
  ///     производить сортировку по указанному диапазону. При назначении диапазона
  ///     соблюдаются следующие требования:
  ///         1. 0 <= loSortBorder < count(), в противном случае LoSortBorder
  ///            выставляется в 0.
  ///         2. loSortBorder < hiSortBorder <= count(), в противном случае
  ///            hiSortBorder выставляется в count (количество элементов в списке).
  ///     Значения по умолчанию равные -1 предполагают сортировку по всему диапазону.
  template<typename UCompare>
  void sort(UCompare& compare,
            SortMode sortMode = SortUp, void *extParam = 0,
            int loSortBorder = 0, int hiSortBorder = int(-1));

  /// @brief Функция сортировки, используется стратегия сортировки по умолчанию.
  ///
  void sort(SortMode sortMode = SortUp, void *extParam = 0,
            int loSortBorder = 0, int hiSortBorder = int(-1));

  /// @brief Функция для расширенной сортировки.
  ///
  /// К имени функции добавлен индекс "2", это сделано для того чтобы
  /// избежать конфликта имен в компиляторе от Borland.
  /// Объект стратегии сортировки UCompare создается внутри функции.
  template<typename UCompare>
  void sort2(SortMode sortMode = SortUp, void *extParam = 0,
             int loSortBorder = 0, int hiSortBorder = int(-1));

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

  //template<typename UCompare>
  //void QuickSort(T** sortList, int L, int R,
  //               UCompare& compare, SortMode sortMode, void *extParam);
  template<typename UCompare>
  void QSort(T** sortList, int L, int R,
             UCompare& compare, SortMode sortMode, void *extParam);

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

#define DECL_IMPL_CUSTLIST(TYPE_) \
  template<typename T, typename Compare, typename Allocator> \
  TYPE_ CustomList<T, Compare, Allocator>

#define DECL_IMPL_CUSTLIST_SUBTMPL1(TYPE_, SUBT1) \
  template<typename T, typename Compare, typename Allocator> \
    template<typename SUBT1> \
  TYPE_ CustomList<T, Compare, Allocator>

#define DECL_IMPL_CUSTLIST_SUBTMPL2(TYPE_, SUBT1, SUBT2) \
  template<typename T, typename Compare, typename Allocator> \
    template<typename SUBT1, typename SUBT2> \
  TYPE_ CustomList<T, Compare, Allocator>



//DECL_IMPL_CUSTLIST_CONSTR::CustomList(bool container, const Allocator& allocator)
//{
//  //d = new Data<T>();
//    d = new DataType();

//  //d->allocator = allocator_;
//  init(container, allocator);
//}

// DECL_IMPL_CUSTLIST_CONSTR::CustomList(const CustomListType& list)
// {
//   // В функции init() выставляем входящий параметр container_ в TRUE,
//   // делаем это только потому, что входящий параметр - обязательный,
//   // в функции assign() этот парамтер может быть переопределен входящим
//   // списком list.
//   d_ = new Data<T>();
//   init(/*container_*/ true, Allocator());
//   assign(list);
// }

// DECL_IMPL_CUSTLIST_CONSTR::CustomList(CustomListType&& list)
// {
//   //move(list);
//   d = list.d;
//   list.d = 0;
// }

//DECL_IMPL_CUSTLIST_DESTR::~CustomList()
//{
//  if (d) {
//    internalClear();
//    delete [] d->list;
//    delete d;
//  }
//  //delete [] m_list;
//}

DECL_IMPL_CUSTLIST(int)::indexOf(const T* item) const
{
  int index;
  indexOf2(item, index);
  return index;
}

DECL_IMPL_CUSTLIST(bool)::indexOf2(const T* item, int& index) const
{
  //CHECK_INTERNAL_DATA_PTR

  T** item_ = listBegin();
  T** end_  = listEnd();
  while (item_ != end_)
  {
    if (*item_ == item)
    {
      index = int(item_ - listBegin());
      return true;
    }
    ++item_;
  }
  //for (int i = 0; i < FCount; ++i)
  //  if (FList[i] == item) return i;
  index = int(-1);
  return false;
}

////DECL_IMPL_CUSTLIST(void)::clear()
//DECL_IMPL_CUSTLIST(void)::internalClear()
//{
//  CHECK_INTERNAL_DATA_PTR(d)

//  if (d->container)
//  {
//    T** item_ = listBegin();
//    T** end_  = listEnd();
//    while (item_ != end_)
//    {
//      if (*item_)
//        d->allocator.destroy(*item_);
//      ++item_;
//    }
//  }
//  d->count = 0;
//  d->sortState = NoSorted;
//}

// --- Первая группа функций поиска ---
DECL_IMPL_CUSTLIST_SUBTMPL1(FindResult, U)::find(const U* item,
                                                 bool bruteForce,
                                                 int startFindIndex,
                                                 void* extParam) const
{
  return find<U, Compare>(item, compare(), bruteForce, startFindIndex, extParam);
}

DECL_IMPL_CUSTLIST_SUBTMPL1(T*, U)::findItem(const U* item,
                                             bool bruteForce,
                                             int startFindIndex,
                                             void* extParam) const
{
  FindResult fr = find<U, Compare>(item, compare(), bruteForce, startFindIndex, extParam);
  return (fr.success()) ? d->list[fr.index()] : 0;
}

DECL_IMPL_CUSTLIST_SUBTMPL1(FindResult, U)::findRef(const U& item,
                                                    bool bruteForce,
                                                    int startFindIndex,
                                                    void* extParam) const
{
  return find<U, Compare>(&item, compare(), bruteForce, startFindIndex, extParam);
}

DECL_IMPL_CUSTLIST_SUBTMPL1(T*, U)::findRefItem(const U& item,
                                                bool bruteForce,
                                                int startFindIndex,
                                                void* extParam) const
{
  return findItem<U, Compare>(&item, compare(), bruteForce, startFindIndex, extParam);
}

//// --- Вторая группа функций поиска ---
//DECL_IMPL_CUSTLIST_SUBTMPL2(FindResult, UCompare, U)::find(const U *item,
//                                                           bool bruteForce,
//                                                           int startFindIndex,
//                                                           void *extParam) const
//{
//  UCompare u_compare;
//  return find<U, UCompare>(item, u_compare, bruteForce, startFindIndex, extParam);
//}

//DECL_IMPL_CUSTLIST_SUBTMPL2(T*, UCompare, U)::findItem(const U* item,
//                                                       bool bruteForce,
//                                                       int startFindIndex,
//                                                       void* extParam) const
//{
//  UCompare u_compare;
//  FindResult fr = find<U, UCompare>(item, u_compare, bruteForce, startFindIndex, extParam);
//  return (fr.success()) ? d->list[fr.index()] : 0;
//}

//DECL_IMPL_CUSTLIST_SUBTMPL2(FindResult, UCompare, U)::findRef(const U& item,
//                                                              bool bruteForce,
//                                                              int startFindIndex,
//                                                              void* extParam) const
//{
//  UCompare u_compare;
//  return find<U, UCompare>(&item, u_compare, bruteForce, startFindIndex, extParam);
//}

//DECL_IMPL_CUSTLIST_SUBTMPL2(T*, UCompare, U)::findRefItem(const U& item,
//                                                          bool bruteForce,
//                                                          int startFindIndex,
//                                                          void* extParam) const
//{
//  UCompare u_compare;
//  return findItem<U, UCompare>(&item, u_compare, bruteForce, startFindIndex, extParam);
//}

// --- Третья группа функций поиска ---
DECL_IMPL_CUSTLIST_SUBTMPL2(T*, U, UCompare)::findItem(const U* item,
                                                       const UCompare& u_compare,
                                                       bool bruteForce,
                                                       int startFindIndex,
                                                       void* extParam) const
{
  FindResult fr = find<U, UCompare>(item, u_compare, bruteForce, startFindIndex, extParam);
  return (fr.success()) ? d->list[fr.index()] : 0;
}

DECL_IMPL_CUSTLIST_SUBTMPL2(FindResult, U, UCompare)::findRef(const U& item,
                                                              const UCompare& u_compare,
                                                              bool bruteForce,
                                                              int startFindIndex,
                                                              void* extParam) const
{
  return find<U, UCompare>(&item, u_compare, bruteForce, startFindIndex, extParam);
}

DECL_IMPL_CUSTLIST_SUBTMPL2(T*, U, UCompare)::findRefItem(const U &item,
                                                          const UCompare &u_compare,
                                                          bool bruteForce,
                                                          int startFindIndex,
                                                          void *extParam) const
{
  return findItem<U, UCompare>(&item, u_compare, bruteForce, startFindIndex, extParam);
}

DECL_IMPL_CUSTLIST_SUBTMPL2(FindResult, U, UCompare)::find(const U* item,
                                                           const UCompare& u_compare,
                                                           bool bruteForce,
                                                           int startFindIndex,
                                                           void* extParam) const
{
  CHECK_INTERNAL_DATA_PTR(d)

  try
  {
    if (d->count == 0)
      return FindResult(false, BRUTE_FORCE, 0);

    if (startFindIndex >= d->count)
      return FindResult(false, BRUTE_FORCE, d->count);


    if (/*(FCount < 4)*/ // Нельзя использовать данное условие для отсортированного
                         // списка, т.к. результат поиска (FindResult::index()) будет
                         // содержать неверное значение. Использование данного значения
                         // в функции addInSort() для отсортированного списка приведет
                         // к тому, что список окажется неотсортированным, но флаг
                         // состояния сортировки (sortState) при этом сброшен не будет.
      bruteForce
      || (d->sortState == NoSorted)
      || (d->sortState == CustomUpSorted)
      || (d->sortState == CustomDownSorted))
    {
      T** item_ = listBegin() + startFindIndex;
      T** end_ = listEnd();
      // При сравнении item_ и end_ не использовать оператор !=, т.к. потенциально
      // может возникнуть ситуация, когда item_ изначально окажется больше end_,
      // т.е. если startFindIndex окажется  больше чем число элементов в списке,
      // то условие item_ != end_ никогда не наступит.
      while (item_ < /*не использовать оператор != */ end_)
      {
        if (u_compare(item, *item_, extParam) == 0)
          return FindResult(true, BRUTE_FORCE, int(item_ - listBegin()));
        ++item_;
      }
      return FindResult(false, BRUTE_FORCE, d->count);
    }

    //register intptr_t low = (startFindIndex - 1);
    int low = (startFindIndex);
    int high = d->count;
    int mid;
    int result;
    T **item_, **end_;
    SortState sort_state = (bruteForce) ? NoSorted : d->sortState;
    switch (sort_state)
    {
      case UpSorted:
        while (true)
        {
          mid = (low + high) >> 1;
          result = u_compare(item, d->list[mid], extParam);
          if (result < 0)
            high = mid;
          else if (result > 0)
            //low = mid; Это присвоение приводит к зацикливанию когда начинает
            //           выполняться условие (low + high) / 2 == mid
            low = (low == mid) ? mid + 1 : mid;
          else
            return FindResult(true, !BRUTE_FORCE, mid); //совпадение

          //if ((high - low) <= 1) При такой проверке не находятся граничные значения
          if ((high - low) < 1)
            return FindResult(false, !BRUTE_FORCE, (result > 0) ? mid + 1 : mid);
        }
      case DownSorted:
        while (true)
        {
          mid = (low + high) >> 1;
          result = u_compare(item, d->list[mid], extParam);
          if (result > 0)
            high = mid;
          else if (result < 0)
            //low = mid; Это присвоение приводит к зацикливанию когда начинает
            //           выполняться условие (low + high) / 2 == mid
            low = (low == mid) ? mid + 1 : mid;
          else
            return FindResult(true, !BRUTE_FORCE, mid); //совпадение

          //if ((high - low) <= 1) При такой проверке не находятся граничные значения
          if ((high - low) < 1)
            return FindResult(false, !BRUTE_FORCE, (result < 0) ? mid + 1 : mid);
        }
      default:
        item_ = listBegin() + startFindIndex;
        end_ = listEnd();
        while (item_ != end_)
        {
          if (u_compare(item, *item_, extParam) == 0)
            return FindResult(true, BRUTE_FORCE, int(item_ - listBegin()));
          ++item_;
        }
        //return FindResult(false, BRUTE_FORCE, d->count);
    }
  }
  catch (BreakCompare &)
  {}
  return FindResult(false, BRUTE_FORCE, d->count);
}


#undef DECL_IMPL_CUSTLIST_CONSTR
#undef DECL_IMPL_CUSTLIST_DESTR
#undef DECL_IMPL_CUSTLIST
#undef DECL_IMPL_CUSTLIST_SUBTMPL1
#undef DECL_IMPL_CUSTLIST_SUBTMPL2




//------------------------- Implementation List -----------------------------

#define DECL_IMPL_LIST_CONSTR \
  template<typename T, typename Compare, typename Allocator> \
  List<T, Compare, Allocator>

#define DECL_IMPL_LIST_DESTR  DECL_IMPL_LIST_CONSTR

#define DECL_IMPL_LIST(TYPE_) \
  template<typename T, typename Compare, typename Allocator> \
  TYPE_ List<T, Compare, Allocator>

#define DECL_IMPL_LIST_SUBTMPL1(TYPE_, SUBT1) \
  template<typename T, typename Compare, typename Allocator> \
    template<typename SUBT1> \
  TYPE_ List<T, Compare, Allocator>

#define DECL_LIST_SELFTYPE List<T, Compare, Allocator>


/////////////////////////////
//DECL_IMPL_CUSTLIST_CONSTR::CustomList(bool container_, Allocator* allocator_)
//{
//    init(container_, allocator_);
//}
//
//DECL_IMPL_CUSTLIST_CONSTR::CustomList(const CustomListType &list)
//{
//    // В функции init() выставляем входящий параметр container_ в TRUE,
//    // делаем это только потому, что входящий параметр - обязательный,
//    // в функции assign() этот парамтер может быть переопределен входящим
//    // списком list.
//    init(/*container_*/ true, 0);
//    assign(list);
//}
//////////////////////////

DECL_IMPL_LIST_CONSTR::List(bool container)
{
  init(container);
}

DECL_IMPL_LIST_CONSTR::List(const Allocator& allocator, bool container)
{
  init(container);
  setAllocator(allocator);
}

//DECL_IMPL_LIST_CONSTR::List(const CustomListType& list)
//{
//  init(CONTAINER_CLASS);
//  assign(list);
//}

//DECL_IMPL_LIST_CONSTR::List(const SelfListType& list)
//{
//  init(CONTAINER_CLASS);
//  assign(list);
//}

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

DECL_IMPL_LIST(T*)::add(T *item)
{
  DataType* d = d_func();
  setSortState(NoSorted);
  if (d->count == d->capacity)
    grow();

  d->list[d->count] = item;
  //setCount(count() + 1);
  //incrementCount();
  ++d->count;
  return item;
}

// DECL_IMPL_LIST(T*)::add(T *item, bool copy)
// {
//   if (copy)
//   {
//     if (!d->container)
//       throw LIST_EXCEPT(ERR_NOCREATEOBJ);
//     item = d->allocator.create(item);
//   }
//   return add(item);
// }

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
    T** item_ = CustomListType::listBegin();
    T** end_  = CustomListType::listEnd();
    while (item_ != end_)
    {
      if (*item_)
        d->allocator.destroy(*item_);
      ++item_;
    }
  }
  d->count = 0;
  d->sortState = NoSorted;
}

DECL_IMPL_LIST(void)::remove(int index, bool compressList_)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  T *item = release(index, compressList_);
  if (d->container && item)
    d->allocator.destroy(item);
}

DECL_IMPL_LIST(int)::removeItem(T *item, bool compressList_)
{
  //intptr_t i = indexOf(item);
  //if (i != -1) remove(i, compressList_);
  //return i;

  int index;
  if (indexOf2(item, index)) {
    remove(index, compressList_);
  }
  return index;
}

DECL_IMPL_LIST(void)::removeLast()
{
  DataType* d = d_func();
  T* item = releaseLast();
  if (d->container && item)
    d->allocator.destroy(item);
}

DECL_IMPL_LIST(void)::removes(int index, int count, bool compressList_)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  count = index + count;
  if (count > d->count)
    count = d->count;

  for (int i = index; i < count; ++i)
  {
    T *item = release(i, !COMPRESS_LIST);
    if (d->container && item)
      d->allocator.destroy(item);
  }
  if (compressList_)
      this->compressList();
}

DECL_IMPL_LIST(void)::replace(int index, T* item, bool keepSortState)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  T** item_ = CustomListType::listBegin() + index;
  T* item_old = *item_;
  *item_ = item;

  if (!keepSortState)
    setSortState(NoSorted);

  if (d->container && item_old)
    d->allocator.destroy(item_old);
}

DECL_IMPL_LIST(T*)::release(int index, bool compressList_)
{
  DataType* d = d_func();
  CHECK_BORDERS(index)

  // Не используем ф-цию memmove, т.к. в некоторых случаях это приводит к утечкам
  // памяти. Выявить закономерности при которых происходят утечки не удалось.
  // Утечки обнаружены с помощью утилиты CodeGuard.

  T** item_ = CustomListType::listBegin() + index;
  T* res = *item_;
  if (compressList_)
  {
    // При сжатии списка сдвигаем все элементы оставшиеся справа
    // от точки удаления элемента на одну позицию влево
    T** end_ = CustomListType::listEnd() - 1;
    while (item_ /*<*/ != end_)
    {
      *item_ = *(item_ + 1); ++item_;
    }
    --d->count;
  }
  else
    *item_ = 0;

  return res;
}

DECL_IMPL_LIST(int)::releaseItem(T *item, bool compressList_)
{
  //intptr_t i = indexOf(item);
  //if (i > -1) release(i, compressList_);
  //return i;

  int index;
  if (indexOf2(item, index)) {
    release(index, compressList_);
    return index;
  }
  return int(-1);
}

DECL_IMPL_LIST(T*)::releaseLast()
{
  DataType* d = d_func();
  if (d->count == 0) return 0;
  T* item = *(CustomListType::listEnd() - 1);
  --d->count;
  return item;
}

DECL_IMPL_LIST(void)::exchange(int index1, int index2)
{
  DataType* d = d_func();
  CHECK_BORDERS(index1)
  CHECK_BORDERS(index2)

  setSortState(NoSorted);
  T** list_ = d->list;
  T* item = list_[index1];
  list_[index1] = list_[index2];
  list_[index2] = item;
}

DECL_IMPL_LIST(T*)::addInSort(T *item, const FindResult& fr)
{
  SortState sort_state = CustomListType::sortState();
  //if ((sort_state == NoSorted)
  //     || (sort_state == CustomUpSorted)
  //     || (sort_state == CustomDownSorted))
  if ((sort_state != UpSorted) && (sort_state != DownSorted))
    return add(item);

  item = insert(item, fr.index());
  if (!fr.bruteForce())
    setSortState(sort_state);

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

DECL_IMPL_LIST(T*)::insert(T *item, int index)
{
  DataType* d = d_func();
  setSortState(NoSorted);
  if ((index < 0) || (index >= d->count))
    return add(item);

  if (d->count == d->capacity)
    grow();

  // Не используем функцию memmove() (cм. комментарии в теле проц. release()).
  T** item_ = CustomListType::listBegin() + index;
  T** end_  = CustomListType::listEnd();
  while (item_ /*<*/ != end_)
  {
    *end_ = *(end_ - 1); --end_;
  }
  *item_ = item;
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
    T *item = release(curIndex);
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
    d->capacity = newCapacity;
    T** temp_list = new T* [d->capacity];
    T** temp_item = temp_list;
    T** item_ = CustomListType::listBegin();
    T** end_  = CustomListType::listEnd();
    while (item_ != end_)
      *temp_item++ = *item_++;

    delete [] d->list;
    d->list = temp_list;
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
  DataType* d = d_func();

  if (this == &list)
      return;

  clear();

//   setCapacity(list.capacity());
//   setCompare(list.compare());
//   setAllocator(list.allocator());
//   setContainer(list.container());
//   setSortState(NoSorted);
//   T** item_s = list.listBegin();
//   T** end_s = list.listEnd();
//   T** item_d = listBegin();
//   const bool container_ = container();
//   while (item_s != end_s)
//   {
//     //FList[i] = (FContainer) ? FAllocator.create(list_.item(i)) : list_.item(i);
//     *item_d = (container_) ? d->allocator.create(*item_s) : *item_s;
//     ++item_d, ++item_s;
//     // Счетчик элементов увеличиваем в цикле, это позволит корректно
//     // освободить ресурсы в случае возникновения исключения
//     setCount(item_s - list.listBegin());
//   }
//   setSortState(list.sortState());


  // Отладить
  break_point

  setCapacity(list.capacity());
  setCompare(list.compare());
  setAllocator(list.allocator());
  setContainer(list.container());
  setCount(list.count());
  setSortState(NoSorted);

  // !!! так делать нельзя
  //*d = *(list.d);

  T** item_s = list.listBegin();
  T** end_s  = list.listEnd();
  T** item_d = CustomListType::listBegin();
  int count = 0;
  //const bool container_ = d->container;
  while (item_s != end_s)
  {
    try
    {
      *item_d = (d->container) ? d->allocator.create(*item_s) : *item_s;
      ++item_d, ++item_s, ++count;
    }
    catch (...)
    {
      //setCount(count_);
      d->count = count;
      throw;
    }
  }
  setSortState(list.sortState());
}

//DECL_IMPL_LIST_SUBTMPL1(void, UCompare)::QuickSort(T** sortList,
//                                                   int L, int R,
//                                                   UCompare& u_compare,
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
//          while (u_compare(sortList[I], P1, extParam) < 0) ++I;
//          while (u_compare(sortList[J], P1, extParam) > 0) --J;
//          break;
//        default:
//          while (u_compare(sortList[I], P1, extParam) > 0) ++I;
//          while (u_compare(sortList[J], P1, extParam) < 0) --J;
//      }
//      if (I <= J)
//      {
//        T1 = sortList[I];
//        sortList[I++] = sortList[J];
//        sortList[J--] = T1;
//      }
//    } while (I < J);
//    if (L < J) QuickSort(sortList, L, J, u_compare, sortMode, extParam);
//    L = I;
//  } while (I <= R);
//}

DECL_IMPL_LIST_SUBTMPL1(void, UCompare)::QSort(T** sortList,
                                               int L, int R,
                                               UCompare& u_compare,
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
        while ((u_compare(sortList[i], x, extParam) < 0) && (i < R)) ++i;
        while ((u_compare(sortList[j], x, extParam) > 0) && (j > L)) --j;
        break;
      default: //SortDown
        while ((u_compare(sortList[i], x, extParam) > 0) && (i < R)) ++i;
        while ((u_compare(sortList[j], x, extParam) < 0) && (j > L)) --j;
    }
    if (i <= j)
    {
      tmp = sortList[i];
      sortList[i] = sortList[j];
      sortList[j] = tmp;
      ++i; if (j > 0) --j;
    }
  } while (i <= j);

  if (L < j) QSort(sortList, L, j, u_compare, sortMode, extParam);
  if (i < R) QSort(sortList, i, R, u_compare, sortMode, extParam);
}

DECL_IMPL_LIST_SUBTMPL1(void, UCompare)::sort(UCompare& u_compare,
                                              SortMode sortMode,
                                              void* extParam,
                                              int loSortBorder,
                                              int hiSortBorder)
{
  DataType* d = d_func();

  // Выставляем флаг сортировки даже если в списке один элемент
  // или нет элементов вовсе. Это необходимо для корректной работы
  // функции addInSort().
  setSortState((sortMode == SortUp) ? UpSorted : DownSorted);
  //

  if (d->list && (d->count > 1))
  {
    try
    {
      //setSortState((sortMode == SortUp) ? UpSorted : DownSorted);
      if (!inRange(loSortBorder, 0, d->count))
        loSortBorder = 0;

      if (!inRange(hiSortBorder, loSortBorder + 1, d->count + 1))
        hiSortBorder = d->count;

      if ((loSortBorder != 0) || (hiSortBorder != d->count))
        setSortState((sortMode == SortUp) ? CustomUpSorted : CustomDownSorted);

      QSort<UCompare>(d->list, loSortBorder, hiSortBorder - 1, u_compare, sortMode, extParam);
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

DECL_IMPL_LIST_SUBTMPL1(void, UCompare)::sort2(SortMode sortMode,
                                               void* extParam,
                                               int loSortBorder,
                                               int hiSortBorder)
{
  UCompare u_compare;
  sort<UCompare>(u_compare, sortMode, extParam, loSortBorder, hiSortBorder);
}

DECL_IMPL_LIST(void)::sort(SortMode sortMode,
                           void* extParam,
                           int loSortBorder,
                           int hiSortBorder)
{
  sort<Compare>(CustomListType::compare(), sortMode, extParam, loSortBorder, hiSortBorder);
}

DECL_IMPL_LIST(void)::swap(SelfListType& list)
{
  DataType* p = list.d;
  list.d = CustomListType::d;
  CustomListType::d = p;
}

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

//template<typename T, typename Compare, typename Allocator>
//void List<T, Compare, Allocator>::add(const CustomListType &list)
//{
//  resetSortState();
//  setCapacity(getCount() + list.getCount());
//  T **list_ = getList();
//  //T **item_ = list_ + getCount();
//  //for (int i = 0; i < list.getCount(); ++i)
//  //{
//  //  *item_ = (getContainer()) ? new T(list.itemRef(i)) : list.item(i);
//  //  ++item_;
//  //  // Счетчик элементов увеличиваем в цикле, это позволит корректно
//  //  // освободить ресурсы в случае возникновения исключения
//  //  setCount(getCount() + 1);
//  //}
//
//  T **item_ = list_ + getCount();
//  int i = 0;
//  //for (int i = 0; i < list.count(); ++i)
//  while (i < list.count())
//  {
//    *item_ = (getContainer()) ? new T(list.itemRef(i)) : list.item(i);
//    ++item_, ++i;
//    // Счетчик элементов увеличиваем в цикле, это позволит корректно
//    // освободить ресурсы в случае возникновения исключения
//    //setCount(getCount() + 1);
//
//    // !!! Проверить реализацию: setCount(i) !!!
//    CHECK_BORDERS(10000000)
//    setCount(i);
//  }
//  //setCount(getCount() + list.getCount());
//}


//template<typename ListT, typename CompareT>
//int beginingFindResult2(const ListT& list,
//                           const CompareT& compare,
//                           const FindResult& fr, void* extParam)
//{
//  if (fr.success())
//  {
//    if (fr.index() == 0)
//      return 0;
//
//    //int low = -1;
//    int low = 0;
//    int high = fr.index();
//    int mid;
//    intptr_t result;
//    while (1)
//    {
//      mid = (low + high) >> 1;
//      result = compare(list.item(fr.index()), list.item(mid), extParam);
//      if (result == 0)
//        high = mid;
//      else
//        low = mid;
//
//      //if ((high - low) <= 1)
//      if ((high - low) < 1)
//        return high;
//    }
//  }
//  return int(-1);
//}



#undef DECL_IMPL_LIST_CONSTR
#undef DECL_IMPL_LIST_DESTR
#undef DECL_IMPL_LIST
#undef DECL_IMPL_LIST_SUBTMPL1
#undef DECL_LIST_SELFTYPE

#undef LIST_EXCEPT
#undef LIST_FUNC_NAME

#undef CHECK_BORDERS
#undef CHECK_NOTLESS
#undef CHECK_INTERNAL_DATA_PTR

#undef DECLSPEC_SELECTANY_LST

} /*namespace lst*/

#pragma GCC diagnostic pop
