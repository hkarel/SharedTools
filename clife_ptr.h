/* clang-format off */
/*****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализованы интеллектуальный указатель предназначенный для работы
  с объектами использующими для управления временем жизни внутренний счетчик
  ссылок.
*****************************************************************************/

#pragma  once

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif

#include <type_traits>

//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wparentheses"

/**
  clife_ptr - реализует интеллектуальный указатель предназначенный для работы
  с объектами использующими для управления временем жизни внутренний счетчик
  ссылок.
  Основное назначение - управление жизнью объектов использующих концепцию
  интерфейсов. Так же использование объектов с внутренним счетчиком ссылок
  целесообразно в тех случаях, когда предъявляются повышенные требования при
  работе с памятью. По сравнению с container_ptr использование clife_ptr поз-
  воляет меньше фрагментировать память, а так же в 2 раза снижает количество
  блокировок при выделении новых сегментов памяти.

  Важно:
    clife_ptr не является потокобезопасным классом. Поэтому если допускается
    изменение clife_ptr одновременно из нескольких потоков, то  необходимо
    выполнять mutex-блокировки или atomic-блокировки при обращении к методам
    clife_ptr.
    Примечание: если сделать все методы clife_ptr потокобезопасными - это
    понизит общую эффективность и быстродействие при использовании clife_ptr.
*/
template <typename T> class clife_ptr
{
public:
    typedef T element_t;
    typedef clife_ptr<T> self_t;

    // Признак, позволяющий отличить данный контейнер от container_ptr
    enum {IsContainerPtr = 0};

public:
    clife_ptr() : _ptr(0) {}

    ~clife_ptr() {
        if (_ptr)
            _ptr->release();
    }

    explicit clife_ptr(T* p) {
        _ptr = p;
        if (_ptr)
            _ptr->add_ref();
    }

    clife_ptr(T* p, bool add_ref) {
        _ptr = p;
        if (_ptr && add_ref) {
            // В зависимости от организации хранимого объекта счетчик ссылок
            // может быть увеличен внутри clife_ptr или снаружи.
            _ptr->add_ref();
        }
    }

    // Дефолтные функции должны быть определены, иначе компилятор создаст их
    // неявно, и их поведение будет отличаться от ожидаемого.
    clife_ptr(const self_t& p) {
        _ptr = 0;
        assign(p);
    }
    self_t& operator= (const self_t& p) {
        assign(p);
        return *this;
    }

    template<typename otherT>
    clife_ptr(const clife_ptr<otherT> & p) {
        _ptr = 0;
        assign(p);
    }
    template<typename otherT>
    self_t& operator= (const clife_ptr<otherT> & p) {
        assign(p);
        return *this;
    }

    clife_ptr(self_t&& p) {
        _ptr = 0;
        assign(p, true);
    }
    self_t& operator= (self_t&& p) {
        assign(p, true);
        return *this;
    }

    template<typename otherT>
    clife_ptr(clife_ptr<otherT> && p) {
        _ptr = 0;
        assign(p, true);
    }
    template<typename otherT>
    self_t& operator= (clife_ptr<otherT> && p) {
        assign(p, true);
        return *this;
    }

    // Динамическое преобразование типа.
    template<typename other_cptrT>
    other_cptrT dynamic_cast_to() const {
        typedef typename other_cptrT::element_t other_element_t;
        // Проверка на эквивалентность типов
        static_assert(std::is_same<other_cptrT, clife_ptr<other_element_t>>::value,
                      "Types must be identical");
        if (!empty()) {
            if (other_element_t* other_element = dynamic_cast<other_element_t*>(get()))
                return other_cptrT(other_element);
        }
        return other_cptrT();
    }

    void release() {if (_ptr) {_ptr->release(); _ptr = 0;}}
    void reset()   {if (_ptr) {_ptr->release(); _ptr = 0;}}

    T*  get() const NOEXCEPT {return _ptr;}

    T* operator-> () const NOEXCEPT {return  _ptr;}
    T& operator*  () const NOEXCEPT {return *_ptr;}
    operator T*   () const NOEXCEPT {return  _ptr;}

    // Допускается использовать только для инициализации.
    T** ref() {
        assert(_ptr == 0);
        return &_ptr;
    }

    void attach(T* p) {
        if (_ptr) _ptr->release();
        _ptr = p;
    }

    T* detach() NOEXCEPT {
        T* p = _ptr; _ptr = 0; return p;
    }

    bool empty()    const NOEXCEPT {return (_ptr == 0);}
    bool is_empty() const NOEXCEPT {return (_ptr == 0);}

    bool operator! () const NOEXCEPT {return (_ptr == 0);}
    operator bool  () const NOEXCEPT {return (_ptr != 0);}

    // Фиктивная функция, введена для обеспечения возможности компиляции
    // шаблонных функций использующих как clife_ptr, так и container_ptr.
    static self_t create_join_ptr() {return self_t();}

private:
    // Используется в обычных операторах присваивания и копирования.
    template<typename otherT>
    void assign(const clife_ptr<otherT> & p) {
        if (_ptr) _ptr->release();
        // Проверка на корректность преобразования типа. Допускается преобразо-
        // вание только от классов-наследников к базовым классам.
        _ptr = p.get();
        if (_ptr)
            _ptr->add_ref();
    }

    // Используется в rvalue-операторах присваивания и копирования.
    template<typename otherT>
    void assign(clife_ptr<otherT> & p, bool /*rvalue*/) {
        if (_ptr) _ptr->release();
        _ptr = p.get();
        p._ptr = 0;
    }

private:
    T* _ptr = {0};

    template <typename> friend class clife_ptr;
};

//#pragma GCC diagnostic pop
